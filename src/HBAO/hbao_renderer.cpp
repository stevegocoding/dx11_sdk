#include "hbao_renderer.h"
#include "mersenne_twister.h" 

// Must match the values from Shaders/Source/HBAO_PS.hlsl
#define RANDOM_TEXTURE_WIDTH 4
#define NUM_DIRECTIONS 8
#define SCALE ((1<<15))

namespace 
{
    const std::wstring hbao_fx_file_name = L"hbao.fx";
	const std::wstring linearize_non_msaa_depth_fx_file = L"linearize_depth_non_msaa.fx";
	const std::wstring resolve_linearize_msaa_nd_fx_file = L"resolve_linearize_nd_msaa.fx"; 
	// const std::wstring blur_composite_fx_file_name = L"blur_composite.fx";
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
                                                    DXGI_FORMAT_R8_UNORM);
        }
        return m_full_res_ao_zbuffer2; 
    }

	texture_2d_ptr& get_full_res_linear_depth_buffer()
	{
		if (!m_full_res_linear_depth_buffer)
		{
			m_full_res_linear_depth_buffer = make_simple_rt(m_render_context, m_full_width, m_full_height, DXGI_FORMAT_R32_FLOAT);
		}
		return m_full_res_linear_depth_buffer; 
	}

	texture_2d_ptr& get_resolved_normal_tex()
	{
		if (!m_resolved_normal_tex)
		{
			m_resolved_normal_tex = make_simple_rt(m_render_context, m_full_width, m_full_height, DXGI_FORMAT_R8G8B8A8_SNORM);
		}
		return m_resolved_normal_tex; 
	}

	texture_2d_ptr& get_resolved_color_tex()
	{
		if (!m_resolved_color_tex)
		{
			m_resolved_color_tex = make_simple_rt(m_render_context, m_full_width, m_full_height, DXGI_FORMAT_R8G8B8A8_UNORM); 
		}
		return m_resolved_color_tex; 
	}

private:
    render_sys_context_ptr m_render_context;
    texture_2d_ptr m_full_res_ao_zbuffer;
    texture_2d_ptr m_full_res_ao_zbuffer2; 
	texture_2d_ptr m_full_res_linear_depth_buffer;
	texture_2d_ptr m_resolved_normal_tex; 
	texture_2d_ptr m_resolved_color_tex;
    
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
		m_lin_depth_params_cb = make_default_cb<lin_depth_data>(m_render_context->get_d3d11_device()); 
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

		m_cb_lin_depth_data.lin_a = m_cb_data.lin_a; 
		m_cb_lin_depth_data.lin_b = m_cb_data.lin_b; 
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

        m_cb_data.focal_len[0] = 1.0f / tanf(m_fovy_rad * 0.5f) * (ao_height / ao_width);
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
		m_render_context->get_d3d11_device_context()->UpdateSubresource(m_lin_depth_params_cb, 0, NULL, &m_cb_lin_depth_data, 0, 0);
    }

    d3d11_buffer_ptr& get_cb() { return m_params_cb; }
	d3d11_buffer_ptr& get_lin_depth_params_cb() { return m_lin_depth_params_cb; }
    
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
	
	
	struct lin_depth_data 
	{
		float lin_a; 
		float lin_b; 
		float padding[2]; 
	};
	lin_depth_data m_cb_lin_depth_data;

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
	d3d11_buffer_ptr m_lin_depth_params_cb; 

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
    
    void set_depth_resolution(unsigned int w, unsigned int h, unsigned int s)
    {
        width = w;
        height = h; 
        sample_count = s;
    }

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

class c_hbao_input_normal_info
{
public:
	c_hbao_input_normal_info()
		: width(0)
		, height(0)
		, sample_count(0)
	{}

	void set_normal_resolution(unsigned int w, unsigned int h, unsigned int s)
	{
		width = w;
		height = h; 
		sample_count = s;
	}
	
	texture_2d_ptr m_input_msaa_normal_tex; 
	texture_2d_ptr m_input_resolved_normal_tex;
	unsigned int width; 
	unsigned int height; 
	unsigned int sample_count; 
};

class c_hbao_input_color_info
{
public:
	c_hbao_input_color_info()
		: width(0)
		, height(0)
		, sample_count(0)
	{}

	void set_color_resolution(unsigned int w, unsigned int h, unsigned int s)
	{
		width = w;
		height = h; 
		sample_count = s;
	}

	texture_2d_ptr m_input_msaa_color_tex; 
	texture_2d_ptr m_input_resolved_color_tex;
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
	, m_input_normal_tex_sr(NULL)
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
	m_input_normal_info.reset(new c_hbao_input_normal_info()); 
	m_input_color_info.reset(new c_hbao_input_color_info()); 
	m_renderer_options.reset(new c_hbao_renderer_options());
	m_params_cb.reset(new c_hbao_params_constant_buffer(render_sys_context));
	
	hr = compile_effects(); 

	create_random_texture2(); 

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
	
	//////////////////////////////////////////////////////////////////////////
	// HBAO effect 
    hr = D3DX11CompileFromFile(hbao_fx_file_name.c_str(),      // effect file name
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
	m_input_normal_tex_sr = m_hbao_effect_var_table->get_variable_by_name("tex_normal")->AsShaderResource();
    m_input_depth_tex_sr = m_hbao_effect_var_table->get_variable_by_name("tex_linear_depth")->AsShaderResource();
	m_input_random_tex_sr = m_hbao_effect_var_table->get_variable_by_name("tex_random")->AsShaderResource();  
	
	/*
	m_non_linear_depth_tex_sr = m_hbao_effect_var_table->get_variable_by_name("tex_non_linear_depth")->AsShaderResource(); 
	m_non_linear_msaa_depth_tex_sr = m_hbao_effect_var_table->get_variable_by_name("tex_non_linear_msaa_depth")->AsShaderResource();
	*/

	//////////////////////////////////////////////////////////////////////////
	// Linearize non-msaa depth buffer effect 
	effect_blob = NULL;
	error_blob = NULL;
	hr = D3DX11CompileFromFile(
		linearize_non_msaa_depth_fx_file.c_str(),		// effect file name
		NULL,											// defines
		NULL,											// includes
		NULL,											// function name
		"fx_5_0",										// profile
		D3D10_SHADER_DEBUG,								// compile flag
		NULL,											// compile flag
		NULL,											// thread pump
		&effect_blob,									// effect buffer
		&error_blob,									// error buffer
		NULL);
	
	if (FAILED(hr))
	{
		char *buf = (char*)error_blob->GetBufferPointer();
		assert(false && buf); 
	}

	hr = D3DX11CreateEffectFromMemory(
		effect_blob->GetBufferPointer(),         // effect buffer pointer
		effect_blob->GetBufferSize(),
		0,
		m_render_sys_context->get_d3d11_device(),
		&m_linearize_non_msaa_depth_effect);

	m_linearize_non_msaa_depth_effect_var_table.reset(new c_d3dx11_effect_params_table(m_render_sys_context, m_linearize_non_msaa_depth_effect));
	m_linearize_non_msaa_depth_tech = m_linearize_non_msaa_depth_effect_var_table->get_technique_by_name("linearize_non_msaa_depth_tech");
	m_non_linear_depth_tex_sr = m_linearize_non_msaa_depth_effect_var_table->get_variable_by_name("tex_non_linear_depth")->AsShaderResource();
	m_lin_depth_params_cb_a = m_linearize_non_msaa_depth_effect_var_table->get_variable_by_name("cb_linearize_depth_params")->AsConstantBuffer(); 
	

	//////////////////////////////////////////////////////////////////////////
	// Resolve and linearize msaa normal and depth buffer effect
	effect_blob = NULL;
	error_blob = NULL;
	hr = D3DX11CompileFromFile(
		resolve_linearize_msaa_nd_fx_file.c_str(),		// effect file name
		NULL,											// defines
		NULL,											// includes
		NULL,											// function name
		"fx_5_0",										// profile
		D3D10_SHADER_DEBUG,								// compile flag
		NULL,											// compile flag
		NULL,											// thread pump
		&effect_blob,									// effect buffer
		&error_blob,									// error buffer
		NULL);

	if (FAILED(hr))
	{
		char *buf = (char*)error_blob->GetBufferPointer();
		assert(false && buf); 
	}

	hr = D3DX11CreateEffectFromMemory(
		effect_blob->GetBufferPointer(),         // effect buffer pointer
		effect_blob->GetBufferSize(),
		0,
		m_render_sys_context->get_d3d11_device(),
		&m_resolve_linearize_nd_effect);

	m_resolve_linearize_nd_var_table.reset(new c_d3dx11_effect_params_table(m_render_sys_context, m_resolve_linearize_nd_effect));
	m_resolve_linearize_nd_tech = m_resolve_linearize_nd_var_table->get_technique_by_name("resolve_linearize_nd_msaa_tech");
	m_non_linear_msaa_depth_tex_sr = m_resolve_linearize_nd_var_table->get_variable_by_name("tex_non_linear_depth")->AsShaderResource();
	m_normal_msaa_tex_sr = m_resolve_linearize_nd_var_table->get_variable_by_name("tex_msaa_normal")->AsShaderResource(); 
	m_lin_depth_params_cb_b = m_resolve_linearize_nd_var_table->get_variable_by_name("cb_linearize_depth_params")->AsConstantBuffer(); 

	//////////////////////////////////////////////////////////////////////////

	/*
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
	*/
	
    return hr; 
} 

void c_hbao_renderer_component::render_ao(float fovy)
{
	m_render_targets->set_full_resolution(m_input_depth_info->width, m_input_depth_info->height);
    m_params_cb->set_full_resolution(m_input_depth_info->width, m_input_depth_info->height); 
    set_viewport(m_input_depth_info->width, m_input_depth_info->height);

    m_params_cb->set_fovy(fovy);
    m_params_cb->update_ao_resolution_constants(m_renderer_options->m_ao_res_mode);
    m_params_cb->update_scene_scale_constants(); 
    m_params_cb->update_subresource(); 
	
	////////////////////////////////////////////////////////////////////////// 
	// Resolve and linearize depth buffer
	if (m_input_depth_info->sample_count == 1)
	{
		apply_linearize_depth_non_msaa(); 
	}
	else 
	{
		apply_resolve_linearize_normal_depth_msaa();
	}
	
    //////////////////////////////////////////////////////////////////////////
    // HBAO
    apply_render_ao(); 

	//////////////////////////////////////////////////////////////////////////
	// Resolve color 
	if (m_input_depth_info->sample_count > 1)
	{
		ID3D11Resource *src = (ID3D11Resource*)(m_input_color_info->m_input_msaa_color_tex->get_d3d11_texture_2d());
		ID3D11Resource *dest = (ID3D11Resource*)(m_render_targets->get_resolved_color_tex()->get_d3d11_texture_2d());
		m_render_sys_context->get_d3d11_device_context()->
			ResolveSubresource(dest, 
								0, 
								src, 
								0, DXGI_FORMAT_R8G8B8A8_UNORM);
	}

    //////////////////////////////////////////////////////////////////////////
    // Blur
	/*
	if (output_color_rtv)
		apply_render_composit_ps(color_srv, output_color_rtv); 
	*/
}

void c_hbao_renderer_component::set_ao_parameters(hbao_app_params& params)
{
    m_params_cb->set_ao_params(&params);
	m_renderer_options->set_renderer_options(params);
}

void c_hbao_renderer_component::set_normal_depth_for_ao(texture_2d_ptr& input_normal, texture_2d_ptr& input_depth, float z_near, float z_far, float z_min, float z_max, float scene_scale)
{
    D3D11_TEXTURE2D_DESC depth_desc;
	D3D11_TEXTURE2D_DESC normal_desc;
    input_depth->get_d3d11_texture_2d()->GetDesc(&depth_desc); 
	input_normal->get_d3d11_texture_2d()->GetDesc(&normal_desc); 
    // assert(depth_desc.Format != DXGI_FORMAT_R24G8_TYPELESS); 
	assert(depth_desc.SampleDesc.Count == normal_desc.SampleDesc.Count); 

    m_params_cb->set_camera_params(z_near, z_far, z_min, z_max, scene_scale); 
	m_input_depth_info->m_non_linear_depth = input_depth; 
    m_input_depth_info->set_depth_resolution(depth_desc.Width, depth_desc.Height, depth_desc.SampleDesc.Count); 
	m_input_normal_info->set_normal_resolution(normal_desc.Width, normal_desc.Height, normal_desc.SampleDesc.Count); 

	if (normal_desc.SampleDesc.Count == 1)
	{
		m_input_normal_info->m_input_resolved_normal_tex = input_normal; 
	}
	else 
	{
		m_input_normal_info->m_input_msaa_normal_tex = input_normal;
	}
}

void c_hbao_renderer_component::set_color_info(texture_2d_ptr& color_tex)
{
	D3D11_TEXTURE2D_DESC color_desc;
	color_tex->get_d3d11_texture_2d()->GetDesc(&color_desc); 
	m_input_color_info->set_color_resolution(color_desc.Width, color_desc.Height, color_desc.SampleDesc.Count); 
	
	if (color_desc.SampleDesc.Count == 1)
	{
		m_input_color_info->m_input_resolved_color_tex = color_tex; 
	}
	else 
	{
		m_input_color_info->m_input_msaa_color_tex = color_tex; 
	}
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
	PIX_EVENT_BEGIN(APPLY_RENDER_AO);
    // ---------------------------------------------------------------------
    // Input: full resolution linear depth srv (R32_FLOAT)
    // Output: half-res ao buffer (R16_FLOAT) or full-res ao z buffer2 (R16G16_FLOAT)  
    // ---------------------------------------------------------------------
    texture_2d_ptr input_depth;
	texture_2d_ptr input_normal;
    texture_2d_ptr output_ao_z_buf; 
    if (m_renderer_options->m_ao_res_mode == e_half_res_ao)
	{
		
    }
    else
    {
        input_depth = m_render_targets->get_full_res_linear_depth_buffer();  
		input_normal = (m_input_normal_info->sample_count > 1) ? 
						m_render_targets->get_resolved_normal_tex() : m_input_normal_info->m_input_resolved_normal_tex;
        output_ao_z_buf = m_render_targets->get_full_res_ao_zbuffer2(); 
    }
	ID3D11RenderTargetView *rtvs[] = {output_ao_z_buf->get_rtv()};
    m_render_sys_context->get_d3d11_device_context()->OMSetRenderTargets(1, rtvs, NULL);

	m_input_normal_tex_sr->SetResource(input_normal->get_srv());
	m_input_depth_tex_sr->SetResource(input_depth->get_srv());
	m_input_random_tex_sr->SetResource(m_random_texture->get_srv());
	m_hbao_effect_params_cb->SetConstantBuffer(m_params_cb->get_cb());

	m_hbao_default_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context()); 

	m_render_sys_context->get_d3d11_device_context()->Draw(3, 0); 
	
	m_input_normal_tex_sr->SetResource(NULL);
	m_input_depth_tex_sr->SetResource(NULL); 
	m_input_random_tex_sr->SetResource(NULL);
	m_hbao_effect_params_cb->SetConstantBuffer(NULL);

	m_hbao_default_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context());
	
	PIX_EVENT_END();
}

void c_hbao_renderer_component::apply_resolve_linearize_normal_depth_msaa() 
{
	PIX_EVENT_BEGIN(APPLY_RESOLVE_LINEARIZE_NORMAL_DEPTH_MSAA);

	//////////////////////////////////////////////////////////////////////////
	// Make sure the input depth buffer is multi-sampled
	assert(m_input_depth_info->sample_count > 1 && m_input_normal_info->sample_count > 1); 
	
	ID3D11DeviceContext *context = m_render_sys_context->get_d3d11_device_context();
	ID3D11RenderTargetView *rtvs[] = {m_render_targets->get_resolved_normal_tex()->get_rtv(), m_render_targets->get_full_res_linear_depth_buffer()->get_rtv()};
	context->OMSetRenderTargets(2, rtvs, NULL);

	m_non_linear_msaa_depth_tex_sr->SetResource(m_input_depth_info->m_non_linear_depth->get_srv()); 
	m_normal_msaa_tex_sr->SetResource(m_input_normal_info->m_input_msaa_normal_tex->get_srv());
	m_lin_depth_params_cb_b->SetConstantBuffer(m_params_cb->get_lin_depth_params_cb()); 
	m_resolve_linearize_nd_tech->GetPassByIndex(0)->Apply(0, context);

	context->Draw(3, 0); 

	m_non_linear_msaa_depth_tex_sr->SetResource(NULL);
	m_normal_msaa_tex_sr->SetResource(NULL);
	m_lin_depth_params_cb_b->SetConstantBuffer(NULL); 
	m_resolve_linearize_nd_tech->GetPassByIndex(0)->Apply(0, context); 

	PIX_EVENT_END();
}

void c_hbao_renderer_component::apply_linearize_depth_non_msaa()
{ 
	PIX_EVENT_BEGIN(APPLY_LINEARIZE_DEPTH_NON_MSAA);

	//////////////////////////////////////////////////////////////////////////
	// Make sure the input depth buffer is non multi-sampled
	assert(m_input_depth_info->sample_count == 1); 
	
	ID3D11DeviceContext *context = m_render_sys_context->get_d3d11_device_context();
	ID3D11RenderTargetView *rtvs = {m_render_targets->get_full_res_linear_depth_buffer()->get_rtv()};
	context->OMSetRenderTargets(1, &rtvs, NULL);

	m_non_linear_depth_tex_sr->SetResource(m_input_depth_info->m_non_linear_depth->get_srv()); 
	m_lin_depth_params_cb_a->SetConstantBuffer(m_params_cb->get_lin_depth_params_cb());
	m_linearize_non_msaa_depth_tech->GetPassByIndex(0)->Apply(0, context); 

	context->Draw(3,0); 
	
	m_non_linear_depth_tex_sr->SetResource(NULL);
	m_lin_depth_params_cb_a->SetConstantBuffer(NULL); 
	m_linearize_non_msaa_depth_tech->GetPassByIndex(0)->Apply(0, context);

	PIX_EVENT_END();
}

void c_hbao_renderer_component::create_random_texture2()
{
	random_gen.seed((unsigned)0); 

	signed short f[64 * 64 * 4]; 
	for (int i = 0; i < 64 * 64 * 4; i+=4)
	{
		float angle = 2.0f * D3DX_PI * random_gen.randExc() / (float)NUM_DIRECTIONS; 
		f[i  ] = (signed short)(SCALE*cos(angle)); 
		f[i+1] = (signed short)(SCALE*sin(angle)); 
		f[i+2] = (signed short)(SCALE*random_gen.randExc());
		f[i+3] = 0;
	}

	D3D11_TEXTURE2D_DESC desc;
	desc.Width            = 64;
	desc.Height           = 64;
	desc.MipLevels        = 1;
	desc.ArraySize        = 1;
	desc.Format           = DXGI_FORMAT_R16G16B16A16_SNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage            = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags   = 0;
	desc.MiscFlags        = 0;

	D3D11_SUBRESOURCE_DATA sr_desc;
	sr_desc.pSysMem          = f;
	sr_desc.SysMemPitch      = 64*4*sizeof(signed short);
	sr_desc.SysMemSlicePitch = 0;

	m_random_texture.reset(new c_texture2D(m_render_sys_context, &desc, &sr_desc)); 
	m_random_texture->bind_sr_view(NULL); 
}

texture_2d_ptr& c_hbao_renderer_component::get_ao_render_target()
{
	return m_render_targets->get_full_res_ao_zbuffer2();
}

texture_2d_ptr& c_hbao_renderer_component::get_resolved_normal_tex()
{
	return m_render_targets->get_resolved_normal_tex();
}

texture_2d_ptr& c_hbao_renderer_component::get_resolved_depth_tex()
{
	return m_render_targets->get_full_res_linear_depth_buffer();
}

texture_2d_ptr& c_hbao_renderer_component::get_resolved_color_tex()
{	
	return m_render_targets->get_resolved_color_tex();
}