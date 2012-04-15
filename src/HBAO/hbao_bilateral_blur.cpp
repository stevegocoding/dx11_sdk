#include "hbao_bilateral_blur.h"

namespace 
{
	const std::wstring blur_fx_file_name = L"bilateral_blur.fx"; 
	const std::string fx_macro_num_msaa = "NUM_MSAA_SAMPLES";
}

class c_bilateral_blur_params_cb 
{
public:

	c_bilateral_blur_params_cb(const render_sys_context_ptr& render_context)
		: m_render_context(render_context)
	{
		m_params_cb = make_default_cb<data>(m_render_context->get_d3d11_device());
	}

	void set_resolution_params(int width, int height)
	{
		m_cb_data.resolution[0] = (float)width; 
		m_cb_data.resolution[1] = (float)height; 
		
		m_cb_data.inv_resolution[0] = 1.0f / width; 
		m_cb_data.inv_resolution[1] = 1.0f / height; 
	}

	void set_blur_params(const blur_app_params *params)
	{
		m_cb_data.sharpness = params->sharpness; 
		m_cb_data.blur_radius = params->blur_radius; 
		m_cb_data.edge_threshold = params->edge_threshold; 

		float radius = m_cb_data.blur_radius; 
		float sigma = (radius + 1) / 2;
		float inv_sigma2 = 1.0f / (2 * sigma * sigma); 
		m_cb_data.blur_falloff = inv_sigma2;
	}

	void update_subresource()
	{
		m_render_context->get_d3d11_device_context()->UpdateSubresource(m_params_cb, 0, NULL, &m_cb_data, 0, 0); 
	}
	
	d3d11_buffer_ptr& get_cb() 
	{
		return m_params_cb;
	}

protected: 

	struct data
	{
		float resolution[2];
		float inv_resolution[2]; 
		float blur_radius;
		float blur_falloff; 
		float sharpness;
		float edge_threshold;
	};

	data m_cb_data; 
	
	d3d11_buffer_ptr m_params_cb;
	
	render_sys_context_ptr m_render_context;
};

c_bilateral_blur::c_bilateral_blur()
	: m_blur_tech(NULL)
	, m_edge_detect_tech(NULL)
	, m_src_tex_sr(NULL)
	, m_depth_tex_sr(NULL)
	, m_num_samples(1)
{
	super(); 
}

c_bilateral_blur::~c_bilateral_blur()
{


}

HRESULT c_bilateral_blur::compile_effect()
{
	HRESULT hr = E_FAIL;

	std::stringstream ss;
	ss << m_num_samples;
	std::string num_samples_str = ss.str();
	D3D10_SHADER_MACRO effect_macros[2];
	memset(effect_macros, 0, sizeof(effect_macros));
	effect_macros[0].Name = fx_macro_num_msaa.c_str();
	effect_macros[0].Definition = num_samples_str.c_str();

	d3d11_blob_ptr effect_blob, error_blob;
	hr = D3DX11CompileFromFile(
		blur_fx_file_name.c_str(),      // effect file name
		effect_macros,					// defines
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

	if (m_blur_effect)
		m_blur_effect.Release(); 
	
	hr = D3DX11CreateEffectFromMemory(
		effect_blob->GetBufferPointer(),         // effect buffer pointer
		effect_blob->GetBufferSize(),
		0,
		m_render_sys_context->get_d3d11_device(),
		&m_blur_effect);

	m_blur_effect_params.reset(new c_d3dx11_effect_params_table(m_render_sys_context, m_blur_effect));
	
	m_blur_tech = m_blur_effect_params->get_technique_by_name("blur_pass_tech"); 
	m_blur_ss_tech = m_blur_effect_params->get_technique_by_name("blur_pass_ss_tech"); 
	m_blur_with_diffuse_tech = m_blur_effect_params->get_technique_by_name("blur_pass_with_diffuse_tech");
	m_blur_with_diffuse_ss_tech = m_blur_effect_params->get_technique_by_name("blur_pass_with_diffuse_ss_tech");
	m_edge_detect_tech = m_blur_effect_params->get_technique_by_name("blur_edge_detection"); 
	m_pass_through_tech = m_blur_effect_params->get_technique_by_name("blur_pass_through_tech"); 
	
	m_depth_tex_sr = m_blur_effect_params->get_variable_by_name("tex_depth")->AsShaderResource(); 
	m_src_tex_sr = m_blur_effect_params->get_variable_by_name("tex_source")->AsShaderResource(); 
	m_color_tex_sr = m_blur_effect_params->get_variable_by_name("tex_color")->AsShaderResource(); 
	m_depth_msaa_tex_sr = m_blur_effect_params->get_variable_by_name("tex_msaa_depth")->AsShaderResource();
	
	m_effect_params_cb = m_blur_effect_params->get_variable_by_name("cb0")->AsConstantBuffer();
	
	return S_OK;
}

HRESULT c_bilateral_blur::create_ds_states()
{
	HRESULT hr; 
	
	D3D11_DEPTH_STENCIL_DESC ds_desc; 
	::memset(&ds_desc, 0, sizeof(D3D11_DEPTH_STENCIL_DESC)); 
	ds_desc.DepthEnable = false;
	ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; 
	
	ds_desc.StencilEnable = true;
	ds_desc.StencilReadMask = 0x0;
	ds_desc.StencilWriteMask = 0xff; 
	
	// Stencil operations if pixel is front-facing
	ds_desc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_REPLACE ;
	ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE ;
	ds_desc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_REPLACE ;
	ds_desc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
	ds_desc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_REPLACE ;
	ds_desc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_REPLACE ;
	ds_desc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_REPLACE ;
	ds_desc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;
	
	m_ds_stencil_write_state = NULL; 
	hr = m_render_sys_context->get_d3d11_device()->CreateDepthStencilState(&ds_desc, &m_ds_stencil_write_state);
	
	ds_desc.DepthEnable = false;
	ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; 
	
	ds_desc.StencilEnable    = TRUE;
	ds_desc.StencilReadMask  = 0xFF;
	ds_desc.StencilWriteMask = 0x0;

	// Stencil operations if pixel is front-facing
	ds_desc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
	ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	ds_desc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
	ds_desc.FrontFace.StencilFunc        = D3D11_COMPARISON_EQUAL;
	ds_desc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
	ds_desc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_KEEP;
	ds_desc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
	ds_desc.BackFace.StencilFunc         = D3D11_COMPARISON_EQUAL;
	
	m_ds_stencil_test_state = NULL; 
	hr = m_render_sys_context->get_d3d11_device()->CreateDepthStencilState(&ds_desc, &m_ds_stencil_test_state);	

	return hr;
}

HRESULT c_bilateral_blur::on_create_resource(const render_sys_context_ptr& render_sys_context)
{
	HRESULT hr;

	super::on_create_resource(render_sys_context);

	m_blur_params_cb.reset(new c_bilateral_blur_params_cb(m_render_sys_context));

	hr = compile_effect(); 

	create_ds_states();
	
	return S_OK; 
}

void c_bilateral_blur::on_release_resource()
{
	m_blur_params_cb.reset();
	m_temp_buf_tex.reset();
	m_edge_stencil_ao.reset();
	m_edge_blurred_ao.reset();

	m_input_src_tex.reset(); 
	m_input_diffuse_tex.reset(); 
	m_input_depth_tex.reset();

	m_ds_stencil_test_state.Release();
	m_ds_default_state.Release();
	m_ds_stencil_write_state.Release(); 
}

void c_bilateral_blur::on_frame_render(double time, float elapsed_time)
{	
	ID3D11DeviceContext *context = m_render_sys_context->get_d3d11_device_context();

	ID3DX11EffectTechnique *current_tech = NULL;
	ID3DX11EffectTechnique *current_tech_ss = NULL;

	if (m_input_diffuse_tex)
	{
		current_tech = m_blur_with_diffuse_tech; 
		current_tech_ss = m_blur_with_diffuse_ss_tech; 
	}
	else
	{
		current_tech = m_blur_tech; 
		current_tech_ss = m_blur_ss_tech; 
	}
	
	m_blur_params_cb->update_subresource();
	m_effect_params_cb->SetConstantBuffer(m_blur_params_cb->get_cb());
	current_tech->GetPassByIndex(0)->Apply(0, context); 
	current_tech_ss->GetPassByIndex(0)->Apply(0, context);
	
	// In MSAA mode run an edge detector on the to find which pixels should be
	// solved using supersampling.
	if (m_num_samples > 1)
	{
		ID3D11DepthStencilView *dsv = m_edge_stencil_ao->get_dsv(); 
		context->ClearDepthStencilView(dsv, D3D11_CLEAR_STENCIL, 1.0, 0);
		context->OMGetDepthStencilState(&m_ds_default_state, 0);
	
		m_src_tex_sr->SetResource(m_input_src_tex->get_srv());
		m_depth_tex_sr->SetResource(m_input_depth_tex->get_srv()); 
		m_color_tex_sr->SetResource(m_input_diffuse_tex->get_srv());
		m_depth_msaa_tex_sr->SetResource(m_input_depth_msaa_tex->get_srv()); 

		m_edge_detect_tech->GetPassByIndex(0)->Apply(0, context); 

		// Fill the stencil
		ID3D11DepthStencilState *ds_state = m_ds_stencil_write_state; 
		dsv = m_edge_stencil_ao->get_dsv();
		context->RSSetViewports(1, &m_full_viewport); 
		context->OMSetDepthStencilState(ds_state, 1);
		context->OMSetRenderTargets(0, NULL, dsv);
		context->Draw(3, 0); 
		
		// Blur pass X 
		ds_state = m_ds_stencil_test_state; 
		dsv = m_edge_stencil_ao->get_dsv(); 
		ID3D11RenderTargetView *rtvs = {m_temp_buf_tex->get_rtv()};
		context->OMSetRenderTargets(1, &rtvs, dsv); 
		context->OMSetDepthStencilState(ds_state, 0); 
		current_tech->GetPassByIndex(0)->Apply(0, context);
		context->Draw(3, 0);
	
		ds_state = m_ds_stencil_test_state; 
		context->OMSetDepthStencilState(ds_state, 1);
		current_tech_ss->GetPassByIndex(0)->Apply(0, context);
		context->Draw(3, 0);

		// Blur pass Y
		ID3D11RenderTargetView *rtvs2 = {m_dest_rtv};
		dsv = m_edge_stencil_ao->get_dsv(); 
		ds_state = m_ds_stencil_test_state; 
		context->OMSetRenderTargets(1, &rtvs2, dsv);
		context->OMSetDepthStencilState(ds_state, 0); 
		m_src_tex_sr->SetResource(m_temp_buf_tex->get_srv()); 
		current_tech->GetPassByIndex(1)->Apply(0, context);
		context->Draw(3, 0);
		
		ds_state = m_ds_stencil_test_state;
		context->OMSetDepthStencilState(ds_state, 1); 
		current_tech_ss->GetPassByIndex(1)->Apply(0, context); 
		context->Draw(3, 0); 
		
		// Unbind SRVs to avoid warning from the fx framework
		m_src_tex_sr->SetResource(NULL); 
		m_depth_tex_sr->SetResource(NULL); 
		m_color_tex_sr->SetResource(NULL);
		m_depth_msaa_tex_sr->SetResource(NULL); 
		current_tech->GetPassByIndex(0)->Apply(0, context);
		current_tech_ss->GetPassByIndex(0)->Apply(0, context);
		       
		// Reset the depth-stencil state
		ds_state = m_ds_default_state; 
		context->OMSetDepthStencilState(ds_state, 0);
		m_ds_default_state = NULL; 
	}
	else
	{
		/*
		// Blur Pass X
		context->RSSetViewports(1, &m_full_viewport); 

		ID3D11RenderTargetView *rtvs = {m_temp_buf_tex->get_rtv()};
		context->OMSetRenderTargets(1, &rtvs, NULL); 
		
		m_src_tex_sr->SetResource(m_input_src_tex->get_srv()); 
		m_depth_tex_sr->SetResource(m_input_depth_tex->get_srv()); 
		m_color_tex_sr->SetResource(m_input_depth_tex->get_srv()); 
		current_tech->GetPassByIndex(0)->Apply(0, context); 
		context->Draw(3, 0); 
		
		// Blur Pass Y
		context->RSSetViewports(1, &m_full_viewport); 
		context->OMSetRenderTargets(1, &)

		*/
		
		
	}
}

void c_bilateral_blur::apply_passthrough(ID3D11RenderTargetView *dest_rtv, texture_2d_ptr& diffuse_sr_tex)
{	
	ID3D11DeviceContext *context = m_render_sys_context->get_d3d11_device_context();
	context->RSSetViewports(1, &m_full_viewport); 
	context->OMSetRenderTargets(1, &dest_rtv, NULL); 
	
	if (diffuse_sr_tex->get_srv())
	{

		m_blur_params_cb->update_subresource();
		m_effect_params_cb->SetConstantBuffer(m_blur_params_cb->get_cb());
		m_pass_through_tech->GetPassByIndex(0)->Apply(0,  m_render_sys_context->get_d3d11_device_context());  
		
		// blur passthrough
		ID3D11ShaderResourceView *srv = diffuse_sr_tex->get_srv(); 
		m_color_tex_sr->SetResource(srv); 

		m_pass_through_tech->GetPassByIndex(0)->Apply(0,  m_render_sys_context->get_d3d11_device_context()); 

		m_src_tex_sr->SetResource(m_input_src_tex->get_srv());

		m_pass_through_tech->GetPassByIndex(0)->Apply(0,  m_render_sys_context->get_d3d11_device_context()); 
		
		m_depth_msaa_tex_sr->SetResource(m_input_depth_msaa_tex->get_srv()); 

		m_pass_through_tech->GetPassByIndex(0)->Apply(0,  m_render_sys_context->get_d3d11_device_context()); 
		
		m_pass_through_tech->GetPassByIndex(0)->Apply(0,  m_render_sys_context->get_d3d11_device_context()); 
		context->Draw(3, 0); 
		m_color_tex_sr->SetResource(NULL);
		m_pass_through_tech->GetPassByIndex(0)->Apply(0,  m_render_sys_context->get_d3d11_device_context());
	}
}

void c_bilateral_blur::on_frame_update(double time, float elapsed_time)
{


}

void c_bilateral_blur::create_sr_textures()
{
	// Create non-msaa RT
	if (m_temp_buf_tex)
		m_temp_buf_tex.reset();
	m_temp_buf_tex = make_simple_rt(m_render_sys_context, m_output_width, m_output_height, DXGI_FORMAT_R8_UNORM);

	if (m_edge_blurred_ao)
		m_edge_blurred_ao.reset(); 
	m_edge_blurred_ao = make_simple_rt(m_render_sys_context, m_output_width, m_output_height, DXGI_FORMAT_R8_UNORM);

	if (m_edge_stencil_ao)
		m_edge_stencil_ao.reset(); 
	m_edge_stencil_ao = make_simple_ds(m_render_sys_context, m_output_width, m_output_height, DXGI_FORMAT_R24G8_TYPELESS);  
}

void c_bilateral_blur::set_viewport(int width, int height)
{
	m_full_viewport.TopLeftX = 0.0f;
	m_full_viewport.TopLeftY = 0.0f; 
	m_full_viewport.Width = (float)width; 
	m_full_viewport.Height = (float)height;
	m_full_viewport.MinDepth = 0.0f;
	m_full_viewport.MaxDepth = 1.0f; 
}

void c_bilateral_blur::set_num_samples(int num_samples)
{
	m_num_samples = num_samples; 

	// compile_effect(); 
}

void c_bilateral_blur::set_resources(const texture_2d_ptr& src_tex, 
									 const texture_2d_ptr& diffuse_tex, 
									 const texture_2d_ptr& depth_tex,
									 const texture_2d_ptr& depth_msaa_tex)
{
	m_input_src_tex = src_tex; 
	m_input_diffuse_tex = diffuse_tex; 
	m_input_depth_tex = depth_tex; 
	m_input_depth_msaa_tex = depth_msaa_tex;
}

void c_bilateral_blur::set_resolution_params(int input_width, int input_height, int output_width, int output_height)
{
	m_input_width = input_width; 
	m_input_height = input_height; 
	m_output_width = output_width; 
	m_output_height = output_height;

	m_blur_params_cb->set_resolution_params(m_input_width, m_input_height);
}

void c_bilateral_blur::set_blur_params(const blur_app_params& params)
{
	m_blur_params_cb->set_blur_params(&params); 
}

void c_bilateral_blur::set_dest_rtv(ID3D11RenderTargetView *dest_rtv)
{
	m_dest_rtv = dest_rtv;
}