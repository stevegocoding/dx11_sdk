#include "hbao_resolve.h"
#include "utils.h"

namespace
{
    const std::wstring fx_file_name = L"resolve.fx";
    const std::string fx_macro_num_msaa = "NUM_MSAA_SAMPLES";
    const std::string fx_macro_downsampled = "DOWNSAMPLED";

    struct cb_proj_planes
    {
        float z_near; 
        float z_far;
        float padding[2];
    };
}

c_hbao_resolve_component::c_hbao_resolve_component()
	: m_resolve_linearize_nd_tech(NULL)
	, m_resolve_nd_tech(NULL)
	, m_tex_depth_sr(NULL)
	, m_tex_normal_sr(NULL)
	, m_proj_plane_cb(NULL)
{
	
}

HRESULT c_hbao_resolve_component::on_create_resource(const render_sys_context_ptr& render_sys_context)
{
    super::on_create_resource(render_sys_context);

    m_cb_proj_planes = make_default_cb<cb_proj_planes>(render_sys_context->get_d3d11_device());

    return S_OK;
}

void c_hbao_resolve_component::on_release_resource()
{
    m_rt_resolved_color.reset(); 
    m_rt_resolved_depth.reset(); 
}

void c_hbao_resolve_component::on_swap_chain_resized(const swap_chain_resize_event& event)
{
    m_in_width = event.width;
    m_in_height = event.height; 
    
    if (m_in_width != m_out_width || m_in_height != m_out_height)
        m_need_upsample = true;
    else 
        m_need_upsample = false;
    
    recompile_effect(); 
    recreate_render_textures(); 
}

void c_hbao_resolve_component::set_input_rt_size(int width, int height)
{
    m_in_width = width; 
    m_in_height = height;
    
    m_in_vp.TopLeftX = 0;
    m_in_vp.TopLeftY = 0; 
    m_in_vp.MinDepth = 0; 
    m_in_vp.MaxDepth = 1;
    m_in_vp.Width = (float)width;
    m_in_vp.Height = (float)height;
}

void c_hbao_resolve_component::set_output_rt_size(int width, int height)
{
    m_out_width = width; 
    m_out_height = height;
    
    m_out_vp.TopLeftX = 0; 
    m_out_vp.TopLeftY = 0; 
    m_out_vp.MinDepth = 0; 
    m_out_vp.MaxDepth = 1; 
    m_out_vp.Width = (float)width; 
    m_out_vp.Height = (float)height;
}

HRESULT c_hbao_resolve_component::recreate_render_textures()
{
    D3D11_TEXTURE2D_DESC tex_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    memset(&tex_desc, 0, sizeof(D3D11_TEXTURE2D_DESC));
    memset(&srv_desc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

    tex_desc.Width = m_in_width;
    tex_desc.Height = m_in_height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT; 
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; 
    tex_desc.CPUAccessFlags = 0; 
    tex_desc.MiscFlags = 0;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Texture2D.MostDetailedMip = 0;
    m_rt_resolved_color.reset(new c_texture2D(m_render_sys_context, &tex_desc, NULL));
    m_rt_resolved_color->bind_rt_view(NULL); 
    m_rt_resolved_color->bind_sr_view(&srv_desc);

	tex_desc.Width = m_out_width;
	tex_desc.Height = m_out_height; 
	m_rt_resolved_normal.reset(new c_texture2D(m_render_sys_context, &tex_desc, NULL));
	m_rt_resolved_normal->bind_sr_view(NULL);
	m_rt_resolved_normal->bind_rt_view(NULL);

    tex_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    m_rt_resolved_depth.reset(new c_texture2D(m_render_sys_context, &tex_desc, NULL));
    m_rt_resolved_depth->bind_rt_view(NULL);
    m_rt_resolved_depth->bind_sr_view(&srv_desc); 

    if (m_need_upsample)
    {
        tex_desc.Width = m_in_width;
        tex_desc.Height = m_in_height; 
        tex_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT; 
        m_src_rt_depth.reset(new c_texture2D(m_render_sys_context, &tex_desc, NULL));
        m_src_rt_depth->bind_rt_view(NULL);
        m_src_rt_depth->bind_sr_view(&srv_desc); 
    }
    return S_OK;
}

HRESULT c_hbao_resolve_component::recompile_effect()
{
    HRESULT hr = S_OK;

    ////////////////////////////////////////////////////////////////////////
    // Compile effect
    std::stringstream ss;
    ss << m_num_samples;
    std::string num_samples_str = ss.str();
    D3D10_SHADER_MACRO effect_macros[3];
    memset(effect_macros, 0, sizeof(effect_macros));
    effect_macros[0].Name = fx_macro_num_msaa.c_str();
    effect_macros[0].Definition = num_samples_str.c_str();
    effect_macros[1].Name = fx_macro_downsampled.c_str();
    effect_macros[1].Definition = m_need_upsample? "1" : "0";

    d3d11_blob_ptr effect_blob, error_blob;
    hr = D3DX11CompileFromFile(fx_file_name.c_str(),           // effect file name
                            effect_macros,                  // defines
                            NULL,                           // includes
                            NULL,                           // function name
                            "fx_5_0",                       // profile
                            D3D10_SHADER_DEBUG|D3D10_SHADER_ENABLE_STRICTNESS,             // compile flag
                            NULL,                           // compile flag
                            NULL,                           // thread pump
                            &effect_blob,                   // effect buffer
                            &error_blob,                    // error buffer
                            NULL);
    
    if (FAILED(hr))
    {
        char *buf = (char*)(error_blob->GetBufferPointer());
        std::string str(buf);
    }
    
    // Release the existing effect first
    if (m_resolve_effect)
        m_resolve_effect = NULL;
    
    V(D3DX11CreateEffectFromMemory(effect_blob->GetBufferPointer(),         // effect buffer pointer
        effect_blob->GetBufferSize(),
        0,
        m_render_sys_context->get_d3d11_device(),
        &m_resolve_effect));
    
    m_effect_params_table.reset(new c_d3dx11_effect_params_table(m_render_sys_context, m_resolve_effect));
    m_resolve_nd_tech = m_effect_params_table->get_technique_by_name("resolve_nd_tech");
	m_resolve_linearize_nd_tech = m_effect_params_table->get_technique_by_name("resolve_nd_tech");
	
    m_tex_depth_sr = m_effect_params_table->get_variable_by_name("tex_depth")->AsShaderResource();
	m_tex_normal_sr = m_effect_params_table->get_variable_by_name("tex_normal")->AsShaderResource();
    m_proj_plane_cb = m_effect_params_table->get_variable_by_name("cb_proj_plane")->AsConstantBuffer();

    update_cb_proj_planes(); 

    return hr;
} 

void c_hbao_resolve_component::apply_resolve_normal_depth(const texture_2d_ptr& input_normal, const texture_2d_ptr& input_depth)
{
	HRESULT hr; 

	PIX_EVENT_BEGIN_TECHNIQUE(resolve_normal_depth_tech);
	PIX_EVENT_SET_RENDER_TARGETS()
	ID3D11RenderTargetView *rtvs[] = {m_rt_resolved_normal->get_rtv(), m_rt_resolved_depth->get_rtv()};
	m_render_sys_context->get_d3d11_device_context()->OMSetRenderTargets(2, rtvs, NULL); 

	PIX_EVENT_SET_EFFECT_SHADER_RESOUCE();
	// hr = m_effect_params_table->bind_shader_resource(m_tex_depth_sr, input_depth->get_srv()); 
	// hr = m_effect_params_table->apply_technique(m_resolve_depth_tech); 
	m_tex_normal_sr->SetResource(input_normal->get_srv()); 
	m_tex_depth_sr->SetResource(input_depth->get_srv());
	m_resolve_normal_depth_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context());

	m_render_sys_context->get_d3d11_device_context()->Draw(3, 0);
	PIX_EVENT_END_TECHNIQUE(resolve_normal_depth_tech);

	PIX_EVENT_SET_EFFECT_SHADER_RESOUCE(); 
	m_tex_normal_sr->SetResource(NULL);
	m_tex_depth_sr->SetResource(NULL);
	m_resolve_normal_depth_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context());
}

void c_hbao_resolve_component::update_cb_proj_planes()
{
    cb_proj_planes cb;
    cb.z_near = m_z_near; 
    cb.z_far = m_z_far;
    cb.padding[0] = cb.padding[1] = 0; 
    
    m_render_sys_context->get_d3d11_device_context()->UpdateSubresource(m_cb_proj_planes, 0, 0, &cb, 0, 0);
    m_proj_plane_cb->SetConstantBuffer(m_cb_proj_planes);
    // m_effect_params_table->apply_technique(m_resolve_depth_tech);
	m_resolve_normal_depth_tech->GetPassByIndex(0)->Apply(0, m_render_sys_context->get_d3d11_device_context()); 
}

void c_hbao_resolve_component::resolve_colors(const texture_2d_ptr& input_color)
{
    if (m_num_samples == 1)
    {
        m_rt_resolved_color = input_color;
    }
    else
    {
        // resolve multi-sampled color render target
        m_render_sys_context->get_d3d11_device_context()->
            ResolveSubresource((ID3D11Resource*)(m_rt_resolved_color->get_d3d11_texture_2d()), 0, (ID3D11Resource*)(input_color->get_d3d11_texture_2d()), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    }
}

void c_hbao_resolve_component::resolve_depth(const texture_2d_ptr& input_depth)
{
    texture_2d_ptr linear_depth_rt;
    //if (m_num_samples == 1)
    //{
    //    PIX_EVENT_BEGIN(Begin linearize depth);
    //    apply_linearize_depth(input_depth, linear_depth_rt); 
    //    PIX_EVENT_END();
    //}
    if (input_depth)
    {
        linear_depth_rt = input_depth; 
    }

    PIX_EVENT_BEGIN(Begin resolve nd); 
    
    if (m_num_samples == 1)
    {
        if (!m_need_upsample)
        {
            m_rt_resolved_depth = linear_depth_rt;
        }
        else
        {
            m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &m_out_vp);
            apply_resolve_depth(input_depth);
        }
    }
    else if (m_num_samples > 1)
    {
        if (!m_need_upsample)
            m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &m_out_vp); 
        else 
            m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &m_in_vp);
        
        apply_resolve_depth(input_depth);
    }
    
    PIX_EVENT_END(); 
}

void c_hbao_resolve_component::resolve_normal_depth(const texture_2d_ptr& input_normal, const texture_2d_ptr& input_depth)
{
	texture_2d_ptr linear_depth_rt;
	//if (m_num_samples == 1)
	//{
	//    PIX_EVENT_BEGIN(Begin linearize depth);
	//    apply_linearize_depth(input_depth, linear_depth_rt); 
	//    PIX_EVENT_END();
	//}
	if (input_depth)
	{
		linear_depth_rt = input_depth; 
	}

	PIX_EVENT_BEGIN(Begin resolve nd); 

	if (m_num_samples == 1)
	{
		if (!m_need_upsample)
		{
			m_rt_resolved_depth = linear_depth_rt;
		}
		else
		{
			m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &m_out_vp);
			apply_resolve_normal_depth(input_normal, input_depth);
		}
	}
	else if (m_num_samples > 1)
	{
		if (!m_need_upsample)
			m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &m_out_vp); 
		else 
			m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &m_in_vp);

		apply_resolve_normal_depth(input_normal, input_depth);
	}

	PIX_EVENT_END(); 
}

