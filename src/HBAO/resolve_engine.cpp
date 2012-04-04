#include "resolve_engine.h"

namespace
{
    const std::wstring fx_file_name = L"resolve.fx";
    const std::string fx_macro_num_msaa = "NUM_MSAA_SAMPLES";
    const std::string fx_macro_downsampled = "DOWNSAMPLED";

    std::vector<std::string> tech_names;
    std::vector<std::string> sr_names;
    std::vector<std::string> cb_names;

    enum e_effect_techs
    {
        RESOLVE_ND_TECH = 0,
        LINEARIZE_DEPTH
    };

    enum e_effect_cb_vars
    {
        CB_PROJ_PLANE = 0
    };

    enum e_effect_sr_vars
    {
        SR_NORMAL = 0,
        SR_DEPTH
    };

    __declspec(align(16)) 
    struct cb_proj_planes
    {
        float z_near; 
        float z_far;
    };

    void init_effect_param_names()
    {
        using namespace boost::assign;

        tech_names += "resolve_nd_tech", "linearize_depth_tech";
        sr_names += "tex_normal", "tex_depth";
        cb_names += "cb_proj_plane";
    }
}

HRESULT c_resolve_engine::on_create_device11(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc)
{
    HRESULT hr = S_OK;
   
    init_effect_param_names();
    
    m_device = d3d11_device;

    m_cb_proj_planes = make_default_cb<cb_proj_planes>(d3d11_device);
   
    return hr;
}

HRESULT c_resolve_engine::on_params_changed(ID3D11DeviceContext *device_context, int in_width, int in_height, int num_samples, int out_width, int out_height, float z_near, float z_far)
{
    HRESULT hr = S_OK;
   
    m_in_width = in_width; 
    m_in_height = in_height;
    m_num_samples = num_samples;
    m_out_width = out_width;
    m_out_height = out_height; 
    m_z_near = z_near;
    m_z_far = z_far;
    
    if (m_in_width != m_out_width || m_in_height != m_out_height) 
        m_need_upsample = true; 
    else
        m_need_upsample = false;
    
    recompile_effect();

    //////////////////////////////////////////////////////////////////////////
    // Update params
    cb_proj_planes cb;
    cb.z_near = z_near; 
    cb.z_far = z_far;
    device_context->UpdateSubresource(m_cb_proj_planes, 0, NULL, &cb, 0, 0);
    m_cb_params[0]->SetConstantBuffer(m_cb_proj_planes);
    
    //////////////////////////////////////////////////////////////////////////
    // Re-create resolved render targets
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
    m_rt_resolved_color.reset(new c_render_texture(m_device, &tex_desc, NULL, &srv_desc));
    
    tex_desc.Width = m_out_width;
    tex_desc.Height = m_out_height;
    
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM; 
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM; 
    m_rt_resolved_normal.reset(new c_render_texture(m_device, &tex_desc, NULL, &srv_desc));
    
    tex_desc.Format = DXGI_FORMAT_R32_FLOAT;
    srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
    m_rt_resolved_depth.reset(new c_render_texture(m_device, &tex_desc, NULL, &srv_desc));
    
    if (m_need_upsample)
    {
        tex_desc.Width = m_in_width;
        tex_desc.Height = m_in_height;
        tex_desc.Format = DXGI_FORMAT_R32_FLOAT; 
        m_src_rt_depth.reset(new c_render_texture(m_device, &tex_desc));
    }

    m_in_vp.TopLeftX = 0;
    m_in_vp.TopLeftY = 0;
    m_in_vp.MinDepth = 0;
    m_in_vp.MaxDepth = 1;
    m_in_vp.Width    = (float)m_in_width;
    m_in_vp.Height   = (float)m_in_height;

    m_out_vp.TopLeftX = 0;
    m_out_vp.TopLeftY = 0;
    m_out_vp.MinDepth = 0;
    m_out_vp.MaxDepth = 1;
    m_out_vp.Width    = (float)m_out_width;
    m_out_vp.Height   = (float)m_out_height;
    
    return hr; 
}

void c_resolve_engine::on_destroy_device11()
{
    m_device = NULL;
}

HRESULT c_resolve_engine::resolve_color(ID3D11DeviceContext *device_context, const render_texture_ptr& rt_color, render_texture_ptr& resolved_rt_color)
{
    if (m_num_samples == 1)
    {
        assert(rt_color->m_srv);
        resolved_rt_color = rt_color;
    }
    else 
    {
        device_context->ResolveSubresource((ID3D11Resource*)m_rt_resolved_color->m_texture, 0, (ID3D11Resource*)rt_color->m_texture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        resolved_rt_color = m_rt_resolved_color;
    }
    
    return S_OK;
}

HRESULT c_resolve_engine::resolve_normal_depth(ID3D11DeviceContext *device_context,
                                               const render_texture_ptr& src_rt_normal, 
                                               const render_texture_ptr& src_rt_depth,
                                               render_texture_ptr& resolved_rt_normal,
                                               render_texture_ptr& resolved_rt_depth)
{  
    //////////////////////////////////////////////////////////////////////////
    // Linearize the depth buffer
    d3d11_shader_resourse_view_ptr linear_depth = linearize_depth(device_context, src_rt_depth->m_srv);
    
    // non-MSAA case
    if (m_num_samples == 1)
    {
        if (!m_need_upsample)
        {
            m_rt_resolved_normal->m_srv = src_rt_normal->m_srv;
            m_rt_resolved_depth->m_srv = src_rt_depth->m_srv;
        }
        else 
        {
            device_context->RSSetViewports(1, &m_out_vp);
            resolve_normal_depth_impl(device_context, linear_depth, src_rt_normal->m_srv);      
        }
        return S_OK;
    }
    
    // MSAA
    if (m_num_samples > 1)
    { 
        if (!m_need_upsample) 
            device_context->RSSetViewports(1, &m_out_vp);
        else 
            device_context->RSSetViewports(1, &m_in_vp);

        resolve_normal_depth_impl(device_context, linear_depth, src_rt_normal->m_srv);
    }
    
    resolved_rt_depth = m_rt_resolved_depth;
    resolved_rt_normal = m_rt_resolved_normal;
    
    return S_OK;
}

void c_resolve_engine::resolve_normal_depth_impl(ID3D11DeviceContext *device_context, ID3D11ShaderResourceView *linear_depth, ID3D11ShaderResourceView *normal)
{
    if (normal)
    {
        ID3D11RenderTargetView *rtvs[] = {m_rt_resolved_normal->m_rtv, m_rt_resolved_depth->m_rtv};
        device_context->OMSetRenderTargets(2, rtvs, NULL);
        
        m_sr_params[SR_NORMAL]->SetResource(normal);
        m_sr_params[SR_DEPTH]->SetResource(linear_depth);
        m_tech_vec[RESOLVE_ND_TECH]->GetPassByIndex(0)->Apply(0, device_context);
        
        device_context->Draw(3, 0);
        
        m_sr_params[SR_NORMAL]->SetResource(NULL);
        m_sr_params[SR_DEPTH]->SetResource(NULL);
    }
}

d3d11_shader_resourse_view_ptr& c_resolve_engine::linearize_depth(ID3D11DeviceContext *device_context, d3d11_shader_resourse_view_ptr& src_depth_srv)
{
    render_texture_ptr rt = m_need_upsample ? m_src_rt_depth : m_rt_resolved_depth;
    
    device_context->RSSetViewports(1, &m_in_vp);
    ID3D11RenderTargetView *rtvs[] = {rt->m_rtv};
    device_context->OMSetRenderTargets(1, rtvs, NULL);
    m_sr_params[SR_DEPTH]->SetResource(src_depth_srv);

    m_tech_vec[SR_DEPTH]->GetPassByIndex(0)->Apply(0, device_context); 
    device_context->Draw(3, 0);
    
    m_sr_params[SR_DEPTH]->SetResource(NULL);

    return rt->m_srv;
}

HRESULT c_resolve_engine::recompile_effect()
{
    HRESULT hr = S_OK;

    ////////////////////////////////////////////////////////////////////////
    // Compile effect
    std::stringstream ss;
    ss << m_num_samples; 
    D3D10_SHADER_MACRO effect_macros[3];
    memset(effect_macros, 0, sizeof(effect_macros));
    effect_macros[0].Name = fx_macro_num_msaa.c_str();
    //effect_macros[0].Definition = ss.str().c_str();
    effect_macros[0].Definition = "1"; 
    effect_macros[1].Name = fx_macro_downsampled.c_str();
    effect_macros[1].Definition = m_need_upsample? "1" : "0";
    
    d3d11_blob_ptr effect_blob, error_blob;
    V(D3DX11CompileFromFile(fx_file_name.c_str(),           // effect file name
                            effect_macros,                  // defines
                            NULL,                           // includes
                            NULL,                           // function name
                            "fx_5_0",                       // profile
                            D3D10_SHADER_DEBUG,             // compile flag
                            NULL,                           // compile flag
                            NULL,                           // thread pump
                            &effect_blob,                   // effect buffer
                            &error_blob,                    // error buffer
                            NULL));

    if (m_resolve_effect)
        m_resolve_effect = NULL;
    V(D3DX11CreateEffectFromMemory(effect_blob->GetBufferPointer(),         // effect buffer pointer
                                   effect_blob->GetBufferSize(),
                                   0,
                                   m_device,
                                   &m_resolve_effect));

    m_tech_vec.clear();
    m_cb_params.clear();
    m_sr_params.clear();

    //////////////////////////////////////////////////////////////////////////
    // Get the effect variables
    std::back_insert_iterator<effect_tech_vector> tech_ins = std::back_inserter(m_tech_vec);
    std::back_insert_iterator<effect_cb_param_vector>  cb_ins = std::back_inserter(m_cb_params);
    std::back_insert_iterator<effect_sr_param_vector> sr_ins = std::back_inserter(m_sr_params);
    for (int i = 0; i < (int)tech_names.size(); ++i, ++tech_ins)
        *tech_ins = m_resolve_effect->GetTechniqueByName(tech_names[i].c_str());
    for (int i = 0; i < (int)cb_names.size(); ++i, ++cb_ins)
        *cb_ins = m_resolve_effect->GetConstantBufferByName(cb_names[i].c_str());    
    for (int i = 0; i < (int)sr_names.size(); ++i, ++sr_ins)
        *sr_ins = m_resolve_effect->GetVariableByName(sr_names[i].c_str())->AsShaderResource();
    
    
    return hr;
}