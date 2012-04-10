#include "hbao_renderer.h"
#include "mersenne_twister.h" 

// Must match the values from Shaders/Source/HBAO_PS.hlsl
#define RANDOM_TEXTURE_WIDTH 4
#define NUM_DIRECTIONS 8

namespace 
{
    const std::wstring hbao_fx_file_name = L"hbao.fx";
	const std::wstring blur_composite_fx_file_name = L"blur_composite.fx";
}

MTRand random_gen;

static const float DegToRad = D3DX_PI / 180.0f;

class c_hbao_render_targets
{
public:
    c_hbao_render_targets(const render_sys_context_ptr& render_context)
        : m_render_context(render_context)
        , m_full_width(0)
        , m_full_height(0)
    {}

    void set_full_resolution(unsigned int width, unsigned int height)
    {
        m_full_width = width; 
        m_full_height = height;
    }

    unsigned int get_full_width() const { return m_full_width; }
    unsigned int get_full_height() const { return m_full_height; }

    texture_2d_ptr& get_full_res_ao_zbuffer() 
    {
        if (!m_full_res_ao_zbuffer)
        {
            D3D11_TEXTURE2D_DESC tex_desc;
            memset(&tex_desc, 0, sizeof(D3D11_TEXTURE2D_DESC)); 
            tex_desc.Width = m_full_width; 
            tex_desc.Height = m_full_height; 
            tex_desc.SampleDesc.Count = 1; 
            tex_desc.SampleDesc.Quality = 0;
            tex_desc.MipLevels = 1;
            tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; 
            m_full_res_ao_zbuffer.reset(new c_texture2D(m_render_context, &tex_desc, NULL)); 
        }
        return m_full_res_ao_zbuffer; 
    }

    texture_2d_ptr& get_full_res_ao_zbuffer2()
    {
        if (!m_full_res_ao_zbuffer2)
        {
            m_full_res_ao_zbuffer2 = make_simple_rt(m_render_context, 
                                                    m_full_width,
                                                    m_full_height,
                                                    DXGI_FORMAT_R16G16B16A16_FLOAT);
        }
        return m_full_res_ao_zbuffer2; 
    }

private:
    render_sys_context_ptr m_render_context;
    texture_2d_ptr m_full_res_ao_zbuffer;
    texture_2d_ptr m_full_res_ao_zbuffer2; 
    
    unsigned int m_full_width; 
    unsigned int m_full_height;     
}; 

class c_hbao_params_constant_buffer
{
public:
    c_hbao_params_constant_buffer(const render_sys_context_ptr& render_context)
        : m_render_context(render_context)
    {
		m_params_cb = make_default_cb<data>(m_render_context->get_d3d11_device());
	}

    void set_fovy(float fovy_rad)
    {
        m_fovy_rad = fovy_rad;
    }

    void set_ao_params(const hbao_app_params *params)
    {
        m_radius_scale = max(params->radius, 1.e-8f);  
        m_blur_sharpness = params->blur_sharpness; 
        m_blur_radius = params->blur_radius; 
        m_hbao_shader_id = clamp(params->step_size - 1, 0, NUM_HBAO_PERMUTATIONS-1); 
        m_blur_shader_id = min(m_blur_radius >> 1, NUM_HBAO_PERMUTATIONS-1); 
        
        m_cb_data.pow_exp = params->power_exponent; 
        m_cb_data.strength = params->stength; 

        m_cb_data.angle_bias = params->angle_bias * DegToRad; 
        m_cb_data.tan_angle_bias = tanf(m_cb_data.angle_bias); 
        
        float blur_sigma = ((float)m_blur_radius + 1.0f) * 0.5f; 
        m_cb_data.blur_falloff = INV_LN2 / (2.0f * blur_sigma * blur_sigma);
    }
    
    void set_camera_params(float z_near, float z_far, float z_min, float z_max, float scene_scale)
    {
        m_z_near = z_near; 
        m_z_far = z_far; 
        m_scene_scale = (scene_scale != 0.0f) ? scene_scale : min(z_near, z_far); 

        // The hardware depths are linearized in two steps:
        // 1. Inverse viewport from [zmin,zmax] to [0,1]: z' = (z - zmin) / (zmax - zmin)
        // 2. Inverse projection from [0,1] to [znear,zfar]: w = 1 / (a z' + b)
        
        // w = 1 / [(1/zfar - 1/znear) * z' + 1/znear]
        m_cb_data.lin_a = 1.0f / m_z_far - 1.0f / m_z_near; 
        m_cb_data.lin_b = 1.0f / m_z_near; 
        
        // w = 1 / [(1/zfar - 1/znear) * (z - zmin)/(zmax - zmin) + 1/znear]
        float z_range = z_max - z_min;
        m_cb_data.lin_a /=  z_range;
        m_cb_data.lin_b -= z_min * m_cb_data.lin_a;
    }

    void set_full_resolution(unsigned int width, unsigned int height)
    {
        m_full_width = width;
        m_full_height = height;
        m_cb_data.full_resolution[0] = (float)width; 
        m_cb_data.full_resolution[1] = (float)height; 
        m_cb_data.inv_full_resolution[0] = 1.0f / width; 
        m_cb_data.inv_full_resolution[1] = 1.0f / height;
        m_cb_data.max_radius_pixels = 0.1f * min(m_cb_data.full_resolution[0], m_cb_data.full_resolution[1]);
    }

    void update_ao_resolution_constants(hbao_resolution_mode_enum resolution_mode)
    {
        unsigned int ao_downsample_factor = (resolution_mode == e_half_res_ao)? 2 : 1;
        m_ao_width = m_full_width / ao_downsample_factor;
        m_ao_height = m_full_height / ao_downsample_factor;

        float ao_width = (float)m_ao_width;
        float ao_height = (float)m_ao_height; 
        m_cb_data.ao_resolution[0] = ao_width; 
        m_cb_data.ao_resolution[1] = ao_height;
        
        m_cb_data.inv_ao_resolution[0] = 1.0f / ao_width;
        m_cb_data.inv_ao_resolution[1] = 1.0f / ao_height; 

        m_cb_data.focal_len[0] = 1.0f / tanf(m_fovy_rad * 0.5f) * (ao_height / m_ao_width);
        m_cb_data.focal_len[1] = 1.0f / tanf(m_fovy_rad * 0.5f); 
        m_cb_data.inv_focal_len[0] = 1.0f / m_cb_data.focal_len[0]; 
        m_cb_data.inv_focal_len[1] = 1.0f / m_cb_data.focal_len[1]; 

        m_cb_data.uv_to_view_a[0] = 2.0f * m_cb_data.inv_focal_len[0]; 
        m_cb_data.uv_to_view_a[1] = -2.0f * m_cb_data.inv_focal_len[1];
        m_cb_data.uv_to_view_b[0] = -1.0f * m_cb_data.inv_focal_len[0]; 
        m_cb_data.uv_to_view_b[1] = 1.0f * m_cb_data.inv_focal_len[1];
    }

    void update_scene_scale_constants()
    {
        m_cb_data.r = m_radius_scale * m_scene_scale;
        m_cb_data.r2 = m_cb_data.r * m_cb_data.r; 
        m_cb_data.neg_inv_r2 = -1.0f / m_cb_data.r2;
        m_cb_data.blur_depth_threshold = 2.0f / SQRT_LN2 * (m_scene_scale / m_blur_sharpness);
    }

    void update_subresource()
    {
        m_render_context->get_d3d11_device_context()->UpdateSubresource(m_params_cb, 0, NULL, &m_cb_data, 0, 0);
		
    }

    d3d11_buffer_ptr& get_cb() { return m_params_cb; }
    
protected:

    /** Parameters Notes
    - AO Calculation Parameters     // set_ao_parameters(const hbao_app_params *params)
        - Shader Constant Buffer
            angle_bias
            tan_angle_bias
            pow_exp 
            strength
            blur_falloff
        - App 
            m_radius_scale

    - Camera Parameters             // set_camera_params(float z_near, float z_far, float z_min, float z_max, float scene_scale)
        - Shader Constant Buffer
            lin_a
            lin_b
        - App
            m_z_near
            m_z_far
    
    - Full Resolution Parameters    // set_full_resoltuion(unsigned int width, unsigned int height)
        - Shader Constant Buffer
            full_resolution[2] 
            inv_full_resolution[2]
            max_radius_pixels
        - App
            m_full_width
            m_full_height
           
    - AO Resolution Constants       // update_ao_resolution_constants() 
        - Shader Constant Buffer
            ao_resolution[2]
            inv_ao_resolution[2]
            focal_len[2]
            inv_focal_len[2]
            uv_to_view_a[2]
            uv_to_view_b[2]
        - App 
            m_ao_width
            m_ao_height
            m_scene_scale

    - Scene Scale Constants         // update_scene_scale_constants()
        - Shader Constant Buffer
            r                       // r = m_radius_scale * m_scene_scale
            r2
            neg_inv_r2 
            blur_depth_threshold 

    //////////////////////////////////////////////////////////////////////////
    - Parameters that application controls

        float radius;                           // default = 1.0                    // the actual eye-space AO radius is set to radius*SceneScale
        unsigned int step_size;                 // default = 4                      // step size in the AO pass // 1~16
        float angle_bias;                       // default = 10 degrees             // to hide low-tessellation artifacts // 0.0~30.0
        float stength;                          // default = 1.0                    // scale factor for the AO, the greater the darker
        float power_exponent;                   // default = 1.0                    // the final AO output is pow(AO, powerExponent)
        unsigned int blur_radius;               // default = 16 pixels              // radius of the cross bilateral filter // 0~16
        float blur_sharpness;                   // default = 8.0                    // the higher, the more the blur preserves edges // 0.0~16.0
        bool use_blending;                      // default = true                   // to multiply the AO over the destination render target
    
    */
    struct data
    {
        // Full Resolution Parameters
        float full_resolution[2];
        float inv_full_resolution[2];
        
        // AO Resolution Parameters
        float ao_resolution[2];
        float inv_ao_resolution[2];

        float focal_len[2];
        float inv_focal_len[2]; 
        
        float uv_to_view_a[2];
        float uv_to_view_b[2];

        float r;
        float r2;
        float neg_inv_r2;
        float max_radius_pixels;

        // AO Parameters
        float angle_bias; 
        float tan_angle_bias;
        float pow_exp; 
        float strength;

        float blur_depth_threshold;
        float blur_falloff; 

        // Camera Parameters 
        float lin_a;
        float lin_b; 
    };
    
    data m_cb_data;

    unsigned int m_full_width;
    unsigned int m_full_height;
    unsigned int m_ao_width; 
    unsigned int m_ao_height; 
    float m_radius_scale;
    float m_blur_sharpness; 
    unsigned int m_blur_radius; 
    
    // Camera Parameters
    float m_fovy_rad;
    float m_z_near; 
    float m_z_far; 
    float m_scene_scale;
    
    // Options
    unsigned int m_hbao_shader_id;
    unsigned int m_blur_shader_id;
    
    d3d11_buffer_ptr m_params_cb;

private:
    render_sys_context_ptr m_render_context;
};

//////////////////////////////////////////////////////////////////////////

class c_hbao_input_depth_info
{
public:
    c_hbao_input_depth_info()
        : width(0)
        , height(0)
        , sample_count(1)
    {}
    
    void set_resolution(unsigned int w, unsigned int h, unsigned int s)
    {
        width = w;
        height = h; 
        sample_count = s;
    }

    // ---------------------------------------------------------------------
    /*
    Linear depth texture
    */
    // ---------------------------------------------------------------------
    texture_2d_ptr m_linear_depth; 

    // ---------------------------------------------------------------------
    /*
    Non-linear depth texture
    */
    // ---------------------------------------------------------------------
    texture_2d_ptr m_non_linear_depth; 
    unsigned int width; 
    unsigned int height; 
    unsigned int sample_count; 
}; 
//////////////////////////////////////////////////////////////////////////

class c_hbao_renderer_options
{
public:
    void set_renderer_options(const hbao_app_params& params)
    {
        m_ao_res_mode = params.m_ao_res_mode;
        m_blend_ao = params.use_blending; 
    }
    hbao_resolution_mode_enum m_ao_res_mode;
    bool m_blend_ao;
};

//////////////////////////////////////////////////////////////////////////

c_hbao_renderer_component::c_hbao_renderer_component(float fovy)
    : m_hbao_default_tech(NULL)
    , m_hbao_effect_params_cb(NULL)
    , m_input_depth_tex_sr(NULL)
	, m_input_random_tex_sr(NULL)
{

}

HRESULT c_hbao_renderer_component::on_create_resource(const render_sys_context_ptr& render_sys_context)
{
    HRESULT hr = E_FAIL;

    super::on_create_resource(render_sys_context);

    m_render_targets.reset(new c_hbao_render_targets(render_sys_context));
    
	m_input_depth_info.reset(new c_hbao_input_depth_info());
	m_renderer_options.reset(new c_hbao_renderer_options());
	m_params_cb.reset(new c_hbao_params_constant_buffer(render_sys_context));
	
	hr = compile_effects(); 

	create_random_texture(); 

    return hr;
}

void c_hbao_renderer_component::on_release_resource()
{
    m_params_cb.reset();
}

HRESULT c_hbao_renderer_component::compile_effects()
{
    HRESULT hr = E_FAIL;

    d3d11_blob_ptr effect_blob, error_blob;
    hr = D3DX11CompileFromFile(hbao_fx_file_name.c_str(),           // effect file name
                               NULL,                           // defines
                               NULL,                           // includes
                               NULL,                           // function name
                               "fx_5_0",                       // profile
                               D3D10_SHADER_DEBUG,             // compile flag
                               NULL,                           // compile flag
                               NULL,                           // thread pump
                               &effect_blob,                   // effect buffer
                               &error_blob,                    // error buffer
                               NULL);

    if (FAILED(hr))
    {
        char *buf = (char*)error_blob->GetBufferPointer();
        return hr; 
    }

    hr = D3DX11CreateEffectFromMemory(effect_blob->GetBufferPointer(),         // effect buffer pointer
                                      effect_blob->GetBufferSize(),
                                      0,
                                      m_render_sys_context->get_d3d11_device(),
                                      &m_hbao_effect);

    // Initialize the effect variables 
    m_hbao_effect_var_table.reset(new c_d3dx11_effect_params_table(m_render_sys_context, m_hbao_effect)); 
    m_hbao_default_tech = m_hbao_effect_var_table->get_technique_by_name("hbao_default_tech");
    m_hbao_effect_params_cb = m_hbao_effect_var_table->get_variable_by_name("cb_hbao_params")->AsConstantBuffer(); 
    m_input_depth_tex_sr = m_hbao_effect_var_table->get_variable_by_name("tex_linear_depth")->AsShaderResource();
	m_input_random_tex_sr = m_hbao_effect_var_table->get_variable_by_name("tex_random")->AsShaderResource();  


	//////////////////////////////////////////////////////////////////////////

	effect_blob = NULL;
	error_blob = NULL;
	hr = D3DX11CompileFromFile(blur_composite_fx_file_name.c_str(),           // effect file name
		NULL,                           // defines
		NULL,                           // includes
		NULL,                           // function name
		"fx_5_0",                       // profile
		D3D10_SHADER_DEBUG,             // compile flag
		NULL,                           // compile flag
		NULL,                           // thread pump
		&effect_blob,                   // effect buffer
		&error_blob,                    // error buffer
		NULL);

	if (FAILED(hr))
	{
		char *buf = (char*)error_blob->GetBufferPointer();
		return hr; 
	}

	hr = D3DX11CreateEffectFromMemory(effect_blob->GetBufferPointer(),         // effect buffer pointer
		effect_blob->GetBufferSize(),
		0,
		m_render_sys_context->get_d3d11_device(),
		&m_blur_composite_effect);
	
	m_blur_composite_effect_var_table.reset(new c_d3dx11_effect_params_table(m_render_sys_context, m_blur_composite_effect)); 
	m_blur_composite_tech = m_blur_composite_effect_var_table->get_technique_by_name("blur_composite_tech");
	m_input_ao_tex_sr = m_blur_composite_effect_var_table->get_variable_by_name("tex_ao")->AsShaderResource();
	m_input_color_tex_sr = m_blur_composite_effect_var_table->get_variable_by_name("tex_diffuse")->AsShaderResource();
	
    return hr; 
} 

void c_hbao_renderer_component::render_ao(float fovy, 
										  ID3D11ShaderResourceView *color_srv,
                                          d3d11_render_target_view_ptr& output_color_rtv)
{
	m_render_targets->set_full_resolution(m_input_depth_info->width, m_input_depth_info->height);
    m_params_cb->set_full_resolution(m_input_depth_info->width, m_input_depth_info->height); 
    set_viewport(m_input_depth_info->width, m_input_depth_info->height);

    m_params_cb->set_fovy(fovy);
    m_params_cb->update_ao_resolution_constants(m_renderer_options->m_ao_res_mode);
    m_params_cb->update_scene_scale_constants(); 
    m_params_cb->update_subresource();
	
    //////////////////////////////////////////////////////////////////////////
    // HBAO
    apply_render_ao(); 

    //////////////////////////////////////////////////////////////////////////
    // Blur

	apply_render_composit_ps(color_srv, output_color_rtv); 
	
}

void c_hbao_renderer_component::apply_render_composit_ps(ID3D11ShaderResourceView *color_srv, d3d11_render_target_view_ptr& output_color_rtv)
{
	m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &m_full_viewport); 
	
	ID3D11RenderTargetView *rtvs[] = {output_color_rtv}; 
	
	m_render_sys_context->get_d3d11_device_context()->OMSetRenderTargets(1, rtvs, NULL);
	
	ID3D11ShaderResourceView *srv = m_render_targets->get_full_res_ao_zbuffer2()->get_srv(); 
	ID3D11ShaderResourceView *srv2 = color_srv;
	m_input_ao_tex_sr->SetResource(srv);
	m_input_color_tex_sr->SetResource(srv2);
	
	m_blur_composite_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context());
	
	m_render_sys_context->get_d3d11_device_context()->Draw(3, 0);	

	m_input_ao_tex_sr->SetResource(NULL);
	m_input_color_tex_sr->SetResource(NULL); 

	m_blur_composite_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context());
}

void c_hbao_renderer_component::render_linear_depth()
{
    if (m_input_depth_info->m_linear_depth && m_input_depth_info->sample_count == 1)
    {
		m_linear_depth = m_input_depth_info->m_linear_depth; 
	}
}

void c_hbao_renderer_component::set_ao_parameters(hbao_app_params& params)
{
    m_params_cb->set_ao_params(&params);
	m_renderer_options->set_renderer_options(params);
}

void c_hbao_renderer_component::set_depth_for_ao(texture_2d_ptr& input_depth, float z_near, float z_far, float z_min, float z_max, float scene_scale)
{
    D3D11_TEXTURE2D_DESC desc;
    input_depth->get_d3d11_texture_2d()->GetDesc(&desc); 
    assert(desc.Format != DXGI_FORMAT_R24G8_TYPELESS); 
 
    m_params_cb->set_camera_params(z_near, z_far, z_min, z_max, scene_scale); 
	m_input_depth_info->m_linear_depth = input_depth;
    m_input_depth_info->set_resolution(desc.Width, desc.Height, desc.SampleDesc.Count); 
}

void c_hbao_renderer_component::set_viewport(int width, int height)
{
    m_full_viewport.TopLeftX = 0.0f;
    m_full_viewport.TopLeftY = 0.0f; 
    m_full_viewport.Width = (float)width; 
    m_full_viewport.Height = (float)height;
    m_full_viewport.MinDepth = 0.0f;
    m_full_viewport.MaxDepth = 1.0f;
}

void c_hbao_renderer_component::apply_render_ao()
{
    // ---------------------------------------------------------------------
    // Input: full resolution linear depth srv (R32_FLOAT)
    // Output: half-res ao buffer (R16_FLOAT) or full-res ao z buffer2 (R16G16_FLOAT)  
    // ---------------------------------------------------------------------

    texture_2d_ptr input_depth;
    texture_2d_ptr output_ao_z_buf; 
    if (m_renderer_options->m_ao_res_mode == e_half_res_ao)
	{
		
    }
    else
    {
        input_depth = m_input_depth_info->m_linear_depth;  
        output_ao_z_buf = m_render_targets->get_full_res_ao_zbuffer2(); 
    }
	ID3D11RenderTargetView *rtvs[] = {output_ao_z_buf->get_rtv()};
    m_render_sys_context->get_d3d11_device_context()->OMSetRenderTargets(1, rtvs, NULL);

	m_input_depth_tex_sr->SetResource(input_depth->get_srv());
	m_input_random_tex_sr->SetResource(m_random_texture->get_srv());
	m_hbao_effect_params_cb->SetConstantBuffer(m_params_cb->get_cb());

	m_hbao_default_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context()); 

	m_render_sys_context->get_d3d11_device_context()->Draw(3, 0); 
	
	m_input_depth_tex_sr->SetResource(NULL); 
	m_input_random_tex_sr->SetResource(NULL);
	m_hbao_effect_params_cb->SetConstantBuffer(NULL);

	m_hbao_default_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context()); 
}

void c_hbao_renderer_component::render_blur_x()
{
}

void c_hbao_renderer_component::render_blur_y()
{
}

void c_hbao_renderer_component::create_random_texture()
{ 
	// Mersenne-Twister random numbers in [0,1).
	static const float MersenneTwisterNumbers[1024] = {
		0.463937f,0.340042f,0.223035f,0.468465f,0.322224f,0.979269f,0.031798f,0.973392f,0.778313f,0.456168f,0.258593f,0.330083f,0.387332f,0.380117f,0.179842f,0.910755f,
		0.511623f,0.092933f,0.180794f,0.620153f,0.101348f,0.556342f,0.642479f,0.442008f,0.215115f,0.475218f,0.157357f,0.568868f,0.501241f,0.629229f,0.699218f,0.707733f,
		0.556725f,0.005520f,0.708315f,0.583199f,0.236644f,0.992380f,0.981091f,0.119804f,0.510866f,0.560499f,0.961497f,0.557862f,0.539955f,0.332871f,0.417807f,0.920779f,
		0.730747f,0.076690f,0.008562f,0.660104f,0.428921f,0.511342f,0.587871f,0.906406f,0.437980f,0.620309f,0.062196f,0.119485f,0.235646f,0.795892f,0.044437f,0.617311f,
		0.891128f,0.263161f,0.245298f,0.276518f,0.786986f,0.059768f,0.424345f,0.433341f,0.052190f,0.699924f,0.139479f,0.402873f,0.741976f,0.557978f,0.127093f,0.946352f,
		0.205587f,0.092822f,0.422956f,0.715176f,0.711952f,0.926062f,0.368646f,0.286516f,0.241413f,0.831616f,0.232247f,0.478637f,0.366948f,0.432024f,0.268430f,0.619122f,
		0.391737f,0.056698f,0.067702f,0.509009f,0.920858f,0.298358f,0.701015f,0.044309f,0.936794f,0.485976f,0.271286f,0.108779f,0.325844f,0.682314f,0.955090f,0.658145f,
		0.295861f,0.562559f,0.867194f,0.810552f,0.487959f,0.869567f,0.224706f,0.962637f,0.646548f,0.003730f,0.228857f,0.263667f,0.365176f,0.958302f,0.606619f,0.901869f,
		0.757257f,0.306061f,0.633172f,0.407697f,0.443632f,0.979959f,0.922944f,0.946421f,0.594079f,0.604343f,0.864211f,0.187557f,0.877119f,0.792025f,0.954840f,0.976719f,
		0.350546f,0.834781f,0.945113f,0.155877f,0.411841f,0.552378f,0.855409f,0.741383f,0.761251f,0.896223f,0.782077f,0.266224f,0.128873f,0.645733f,0.591567f,0.247385f,
		0.260848f,0.811970f,0.653369f,0.976713f,0.221533f,0.957436f,0.294018f,0.159025f,0.820596f,0.569601f,0.934328f,0.467182f,0.763165f,0.835736f,0.240033f,0.389869f,
		0.998754f,0.783739f,0.758034f,0.614317f,0.221128f,0.502497f,0.978066f,0.247794f,0.619551f,0.658307f,0.769667f,0.768478f,0.337143f,0.370689f,0.084723f,0.510534f,
		0.594996f,0.994636f,0.181230f,0.868113f,0.312023f,0.480495f,0.177356f,0.367374f,0.741642f,0.202983f,0.229404f,0.108165f,0.098607f,0.010412f,0.727391f,0.942217f,
		0.023850f,0.110631f,0.958293f,0.208996f,0.584609f,0.491803f,0.238266f,0.591587f,0.297477f,0.681421f,0.215040f,0.587764f,0.704494f,0.978978f,0.911686f,0.692657f,
		0.462987f,0.273259f,0.802855f,0.651633f,0.736728f,0.986217f,0.402363f,0.524098f,0.740470f,0.799076f,0.918257f,0.705367f,0.477477f,0.102279f,0.809959f,0.860645f,
		0.118276f,0.009567f,0.280106f,0.948473f,0.025423f,0.458173f,0.512607f,0.082088f,0.536906f,0.472590f,0.835726f,0.078518f,0.357919f,0.797522f,0.570516f,0.162719f,
		0.815968f,0.874141f,0.915300f,0.392073f,0.366307f,0.766238f,0.462755f,0.087614f,0.402357f,0.277686f,0.294194f,0.392791f,0.504893f,0.263420f,0.509197f,0.518974f,
		0.738809f,0.965800f,0.003864f,0.976899f,0.292287f,0.837148f,0.525498f,0.743779f,0.359015f,0.060636f,0.595481f,0.483102f,0.900195f,0.423277f,0.981990f,0.154968f,
		0.085584f,0.681517f,0.814437f,0.105936f,0.972238f,0.207062f,0.994642f,0.989271f,0.646217f,0.330263f,0.432094f,0.139929f,0.908629f,0.271571f,0.539319f,0.845182f,
		0.140069f,0.001406f,0.340195f,0.582218f,0.693570f,0.293148f,0.733441f,0.375523f,0.676068f,0.130642f,0.606523f,0.441091f,0.113519f,0.844462f,0.399921f,0.551049f,
		0.482781f,0.894854f,0.188909f,0.431045f,0.043693f,0.394601f,0.544309f,0.798761f,0.040417f,0.022292f,0.681257f,0.598379f,0.069981f,0.255632f,0.174776f,0.880842f,
		0.412071f,0.397976f,0.932835f,0.979471f,0.244276f,0.488083f,0.313785f,0.858199f,0.390958f,0.426132f,0.754800f,0.360781f,0.862827f,0.526424f,0.090054f,0.673971f,
		0.715044f,0.237489f,0.210234f,0.952837f,0.448429f,0.738062f,0.077342f,0.260666f,0.590478f,0.127519f,0.628981f,0.136232f,0.860189f,0.596789f,0.524043f,0.897171f,
		0.648864f,0.116735f,0.666835f,0.536993f,0.811733f,0.854961f,0.857206f,0.945069f,0.434195f,0.602343f,0.823780f,0.109481f,0.684652f,0.195598f,0.213630f,0.283516f,
		0.387092f,0.182029f,0.834655f,0.948975f,0.373107f,0.249751f,0.162575f,0.587850f,0.192648f,0.737863f,0.777432f,0.651490f,0.562558f,0.918301f,0.094830f,0.260698f,
		0.629400f,0.751325f,0.362210f,0.649610f,0.397390f,0.670624f,0.215662f,0.925465f,0.908397f,0.486853f,0.141060f,0.236122f,0.926399f,0.416056f,0.781483f,0.538538f,
		0.119521f,0.004196f,0.847561f,0.876772f,0.945552f,0.935095f,0.422025f,0.502860f,0.932500f,0.116670f,0.700854f,0.995577f,0.334925f,0.174659f,0.982878f,0.174110f,
		0.734294f,0.769366f,0.917586f,0.382623f,0.795816f,0.051831f,0.528121f,0.691978f,0.337981f,0.675601f,0.969444f,0.354908f,0.054569f,0.254278f,0.978879f,0.611259f,
		0.890006f,0.712659f,0.219624f,0.826455f,0.351117f,0.087383f,0.862534f,0.805461f,0.499343f,0.482118f,0.036473f,0.815656f,0.016539f,0.875982f,0.308313f,0.650039f,
		0.494165f,0.615983f,0.396761f,0.921652f,0.164612f,0.472705f,0.559820f,0.675677f,0.059891f,0.295793f,0.818010f,0.769365f,0.158699f,0.648142f,0.228793f,0.627454f,
		0.138543f,0.639463f,0.200399f,0.352380f,0.470716f,0.888694f,0.311777f,0.571183f,0.979317f,0.457287f,0.115151f,0.725631f,0.620539f,0.629373f,0.850207f,0.949974f,
		0.254675f,0.142306f,0.688887f,0.307235f,0.284882f,0.847675f,0.617070f,0.207422f,0.550545f,0.541886f,0.173878f,0.474841f,0.678372f,0.289180f,0.528111f,0.306538f,
		0.869399f,0.040299f,0.417301f,0.472569f,0.857612f,0.917462f,0.842319f,0.986865f,0.604528f,0.731115f,0.607880f,0.904675f,0.397955f,0.627867f,0.533371f,0.656758f,
		0.627210f,0.223554f,0.268442f,0.254858f,0.834380f,0.131010f,0.838028f,0.613512f,0.821627f,0.859779f,0.405212f,0.909901f,0.036186f,0.643093f,0.187064f,0.945730f,
		0.319022f,0.709012f,0.852200f,0.559587f,0.865751f,0.368890f,0.840416f,0.950571f,0.315120f,0.331749f,0.509218f,0.468617f,0.119006f,0.541820f,0.983444f,0.115515f,
		0.299804f,0.840386f,0.445282f,0.900755f,0.633600f,0.304196f,0.996153f,0.844025f,0.462361f,0.314402f,0.850035f,0.773624f,0.958303f,0.765382f,0.567577f,0.722607f,
		0.001299f,0.189690f,0.364661f,0.192390f,0.836882f,0.783680f,0.026723f,0.065230f,0.588791f,0.937752f,0.993644f,0.597499f,0.851975f,0.670339f,0.360987f,0.755649f,
		0.571521f,0.231990f,0.425067f,0.116442f,0.321815f,0.629616f,0.701207f,0.716931f,0.146357f,0.360526f,0.498487f,0.846096f,0.307994f,0.323456f,0.288884f,0.477935f,
		0.236433f,0.876589f,0.667459f,0.977175f,0.179347f,0.479408f,0.633292f,0.957666f,0.343651f,0.871846f,0.452856f,0.895494f,0.327657f,0.867779f,0.596825f,0.907009f,
		0.417409f,0.530739f,0.547422f,0.141032f,0.721096f,0.587663f,0.830054f,0.460860f,0.563898f,0.673780f,0.035824f,0.755808f,0.331846f,0.653460f,0.926339f,0.724599f,
		0.978501f,0.495221f,0.098108f,0.936766f,0.139911f,0.851336f,0.889867f,0.376509f,0.661482f,0.156487f,0.671886f,0.487835f,0.046571f,0.441975f,0.014015f,0.440433f,
		0.235927f,0.163762f,0.075399f,0.254734f,0.214011f,0.554803f,0.712877f,0.795785f,0.471616f,0.105032f,0.355989f,0.834418f,0.498021f,0.018318f,0.364799f,0.918869f,
		0.909222f,0.858506f,0.928250f,0.946347f,0.755364f,0.408753f,0.137841f,0.247870f,0.300618f,0.470068f,0.248714f,0.521691f,0.009862f,0.891550f,0.908914f,0.227533f,
		0.702908f,0.596738f,0.581597f,0.099904f,0.804893f,0.947457f,0.080649f,0.375755f,0.890498f,0.689130f,0.600941f,0.382261f,0.814084f,0.258373f,0.278029f,0.907399f,
		0.625024f,0.016637f,0.502896f,0.743077f,0.247834f,0.846201f,0.647815f,0.379888f,0.517357f,0.921494f,0.904846f,0.805645f,0.671974f,0.487205f,0.678009f,0.575624f,
		0.910779f,0.947642f,0.524788f,0.231298f,0.299029f,0.068158f,0.569690f,0.121049f,0.701641f,0.311914f,0.447310f,0.014019f,0.013391f,0.257855f,0.481835f,0.808870f,
		0.628222f,0.780253f,0.202719f,0.024902f,0.774355f,0.783080f,0.330077f,0.788864f,0.346888f,0.778702f,0.261985f,0.696691f,0.212839f,0.713849f,0.871828f,0.639753f,
		0.711037f,0.651247f,0.042374f,0.236938f,0.746267f,0.235043f,0.442707f,0.195417f,0.175918f,0.987980f,0.031270f,0.975425f,0.277087f,0.752667f,0.639751f,0.507857f,
		0.873571f,0.775393f,0.390003f,0.415997f,0.287861f,0.189340f,0.837939f,0.186253f,0.355633f,0.803788f,0.029124f,0.802046f,0.248046f,0.354010f,0.420571f,0.109523f,
		0.731250f,0.700653f,0.716019f,0.651507f,0.250055f,0.884214f,0.364255f,0.244975f,0.472268f,0.080641f,0.309332f,0.250613f,0.519091f,0.066142f,0.037804f,0.865752f,
		0.767738f,0.617325f,0.537048f,0.743959f,0.401200f,0.595458f,0.869843f,0.193999f,0.670364f,0.018494f,0.743159f,0.979555f,0.382352f,0.191059f,0.992247f,0.946175f,
		0.306473f,0.793720f,0.687331f,0.556239f,0.958367f,0.390949f,0.357823f,0.110213f,0.977540f,0.831431f,0.485895f,0.148678f,0.847327f,0.733145f,0.397393f,0.376365f,
		0.398704f,0.463869f,0.976946f,0.844771f,0.075688f,0.473865f,0.470958f,0.548172f,0.350174f,0.727441f,0.123139f,0.347760f,0.839587f,0.562705f,0.036853f,0.564723f,
		0.960356f,0.220534f,0.906969f,0.677664f,0.841052f,0.111530f,0.032346f,0.027749f,0.468255f,0.229196f,0.508756f,0.199613f,0.298103f,0.677274f,0.526005f,0.828221f,
		0.413321f,0.305165f,0.223361f,0.778072f,0.198089f,0.414976f,0.007498f,0.464238f,0.785213f,0.534428f,0.060537f,0.572427f,0.693334f,0.865843f,0.034964f,0.586806f,
		0.161710f,0.203743f,0.656513f,0.604340f,0.688333f,0.257211f,0.246437f,0.338237f,0.839947f,0.268420f,0.913245f,0.759551f,0.289283f,0.347280f,0.508970f,0.361526f,
		0.554649f,0.086439f,0.024344f,0.661653f,0.988840f,0.110613f,0.129422f,0.405940f,0.781764f,0.303922f,0.521807f,0.236282f,0.277927f,0.699228f,0.733812f,0.772090f,
		0.658423f,0.056394f,0.153089f,0.536837f,0.792251f,0.165229f,0.592251f,0.228337f,0.147078f,0.116056f,0.319268f,0.293400f,0.872600f,0.842240f,0.306238f,0.228790f,
		0.745704f,0.821321f,0.778268f,0.611390f,0.969139f,0.297654f,0.367369f,0.815074f,0.985840f,0.693232f,0.411759f,0.366651f,0.345481f,0.609060f,0.778929f,0.640823f,
		0.340969f,0.328489f,0.898686f,0.952345f,0.272572f,0.758995f,0.111269f,0.613403f,0.864397f,0.607601f,0.357317f,0.227619f,0.177081f,0.773828f,0.318257f,0.298335f,
		0.679382f,0.454625f,0.976745f,0.244511f,0.880111f,0.046238f,0.451342f,0.709265f,0.784123f,0.488338f,0.228713f,0.041251f,0.077453f,0.718891f,0.454221f,0.039182f,
		0.614777f,0.538681f,0.856650f,0.888921f,0.184013f,0.487999f,0.880338f,0.726824f,0.112945f,0.835710f,0.943366f,0.340094f,0.167909f,0.241240f,0.125953f,0.460130f,
		0.789923f,0.313898f,0.640780f,0.795920f,0.198025f,0.407344f,0.673839f,0.414326f,0.185900f,0.353436f,0.786795f,0.422102f,0.133975f,0.363270f,0.393833f,0.748760f,
		0.328130f,0.115681f,0.253865f,0.526924f,0.672761f,0.517447f,0.686442f,0.532847f,0.551176f,0.667406f,0.382640f,0.408796f,0.649460f,0.613948f,0.600470f,0.485404f,
	};

	int randomNumberIndex = 0;
	std::vector<INT16> dd;
	
	INT16 *data = new INT16[RANDOM_TEXTURE_WIDTH*RANDOM_TEXTURE_WIDTH*4];
	for (UINT i = 0; i < RANDOM_TEXTURE_WIDTH*RANDOM_TEXTURE_WIDTH*4; i += 4)
	{
		assert(randomNumberIndex < 1024);
		float r1 = MersenneTwisterNumbers[randomNumberIndex++];
		float r2 = MersenneTwisterNumbers[randomNumberIndex++];

		// Use random rotatation angles in [0,2P/NUM_HBAO_DIRECTIONS).
		// This looks the same as sampling [0,2PI), but is faster.
		float angle = 2.0f * D3DX_PI * r1 / NUM_DIRECTIONS;
		INT16 x = (INT16)((1<<15)*cos(angle)); 
		INT16 y = (INT16)((1<<15)*sin(angle)); 
		INT16 z = (INT16)((1<<15)*r2);
		
		data[i  ] = x;
		data[i+1] = y;
		data[i+2] = z;
		
		data[i+3] = 0;
		
		dd.push_back(x);
		dd.push_back(y);
		dd.push_back(z);
		dd.push_back(0); 
		
	}

	D3D11_TEXTURE2D_DESC desc;
	desc.Width            = RANDOM_TEXTURE_WIDTH;
	desc.Height           = RANDOM_TEXTURE_WIDTH;
	desc.MipLevels        = 1;
	desc.ArraySize        = 1;
	desc.Format           = DXGI_FORMAT_R16G16B16A16_SNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage            = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags   = 0;
	desc.MiscFlags        = 0;

	D3D11_SUBRESOURCE_DATA srd;
	srd.pSysMem          = data;
	srd.SysMemPitch      = RANDOM_TEXTURE_WIDTH*4*sizeof(INT16);
	srd.SysMemSlicePitch = 0;

	m_random_texture.reset(new c_texture2D(m_render_sys_context, &desc, &srd)); 
	m_random_texture->bind_sr_view(NULL);

	delete[] data;
}
