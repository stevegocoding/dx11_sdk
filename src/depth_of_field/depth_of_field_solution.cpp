#include "depth_of_field_solution.h"

const std::wstring fx_file_name = L"dof.fx";
const std::string render_dof_tech_name = "render_dof";
const std::string cb_var_name = "cb0";
const std::string color_rt_var_name = "g_tex_diffuse";
const std::string depth_rt_var_name = "g_tex_depth";

struct cb0
{
    XMMATRIX mat_inv_proj;
    XMVECTOR vec_depth; 
};

struct fullscreen_quad_vertex
{
    XMFLOAT3         pos;
    XMFLOAT2         texcoord;

    fullscreen_quad_vertex(const XMFLOAT3& _pos, const XMFLOAT2& _texcoord)
        : pos(_pos), texcoord(_texcoord)
    {}
};

c_depth_of_field_solution::c_depth_of_field_solution(const modelview_camera_ptr& camera)
    : m_focal_plane(5.0f)
    , m_screen_width(0.0f)
    , m_screen_height(0.0f)
    , m_camera(camera)
{

}

HRESULT c_depth_of_field_solution::on_d3d11_create_device(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc)
{
    HRESULT hr;

    //////////////////////////////////////////////////////////////////////////
    // Compile effect

    d3d11_blob_ptr effect_blob, error_blob;
    V(D3DX11CompileFromFile(fx_file_name.c_str(),           // effect file name
                            NULL,                           // defines
                            NULL,                           // includes
                            NULL,                           // function name
                            "fx_5_0",                       // profile
                            D3D10_SHADER_DEBUG,             // compile flag
                            NULL,                           // compile flag
                            NULL,                           // thread pump
                            &effect_blob,                   // effect buffer
                            &error_blob,                    // error buffer
                            NULL));

    V(D3DX11CreateEffectFromMemory(effect_blob->GetBufferPointer(),         // effect buffer pointer
                                   effect_blob->GetBufferSize(),
                                   0,
                                   d3d11_device,
                                   &m_dof_effect));

    m_render_dof_tech = m_dof_effect->GetTechniqueByName(render_dof_tech_name.c_str()); 
    m_color_rt_var = m_dof_effect->GetVariableByName(color_rt_var_name.c_str())->AsShaderResource();
    m_depth_rt_var = m_dof_effect->GetVariableByName(depth_rt_var_name.c_str())->AsShaderResource();
    m_cb_var = m_dof_effect->GetConstantBufferByName(cb_var_name.c_str());

    //////////////////////////////////////////////////////////////////////////
    // Setup full-screen quad geometry

    fullscreen_quad_vertex vertices[] =
    {
        fullscreen_quad_vertex(XMFLOAT3(-1, -1, 1), XMFLOAT2(0, 1)),
        fullscreen_quad_vertex(XMFLOAT3(-1, 1, 1), XMFLOAT2(0, 0)),
        fullscreen_quad_vertex(XMFLOAT3(1, -1, 1), XMFLOAT2(1, 1)),
        fullscreen_quad_vertex(XMFLOAT3(1, 1, 1), XMFLOAT2(1, 0))
    };
    // vertex buffer
    D3D11_BUFFER_DESC vb_desc;
    ZeroMemory(&vb_desc, sizeof(D3D11_BUFFER_DESC));
    vb_desc.Usage = D3D11_USAGE_DEFAULT;
    vb_desc.ByteWidth = sizeof(fullscreen_quad_vertex) * 4;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vb_desc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA init_data;
    ZeroMemory(&init_data, sizeof(D3D11_SUBRESOURCE_DATA));
    init_data.pSysMem = vertices;
    V(d3d11_device->CreateBuffer(&vb_desc, &init_data, &m_fullscreen_quad_vb));

    //////////////////////////////////////////////////////////////////////////
    // Create constant buffer

    D3D11_BUFFER_DESC cb_desc;
    cb_desc.Usage = D3D11_USAGE_DEFAULT;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = 0; 
    cb_desc.MiscFlags = 0;
    cb_desc.ByteWidth = sizeof(cb0);
    V(d3d11_device->CreateBuffer(&cb_desc, NULL, &m_cb0)); 

    return S_OK; 
}

void c_depth_of_field_solution::on_d3d11_destroy_device()
{
}

HRESULT c_depth_of_field_solution::on_d3d11_resized_swap_chain(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc)
{
    m_screen_width = (float)backbuf_surface_desc->Width;
    m_screen_height = (float)backbuf_surface_desc->Height;

    //////////////////////////////////////////////////////////////////////////
    // Setup the viewport
    m_vp.Width = (float)backbuf_surface_desc->Width;
    m_vp.Height = (float)backbuf_surface_desc->Height;
    m_vp.MinDepth = 0.0f;
    m_vp.MaxDepth = 1.0f;
    m_vp.TopLeftX = 0;
    m_vp.TopLeftY = 0;
    
    return S_OK;
}

void c_depth_of_field_solution::render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, const render_texture_ptr& color_rt, const render_texture_ptr& depth_rt, double time, float elapsed_time)
{
    float depth_info[4] = { 0.0f, m_screen_width, m_screen_height, m_focal_plane };

    //////////////////////////////////////////////////////////////////////////
    // Set textures

    m_color_rt_var->SetResource(color_rt->m_srv);
    m_depth_rt_var->SetResource(depth_rt->m_srv);
    d3d11_device_context->GenerateMips(color_rt->m_srv);
    
    //////////////////////////////////////////////////////////////////////////
    // Update constant buffer

    cb0 cb;
    XMVECTOR det;
    XMMATRIX proj = convert_d3dxmat_to_xnamat(*(m_camera->GetProjMatrix()));
    cb.vec_depth = XMVectorSet(depth_info[0], depth_info[1], depth_info[2], depth_info[3]);
    cb.mat_inv_proj = XMMatrixTranspose(XMMatrixInverse(&det, proj));
    d3d11_device_context->UpdateSubresource(m_cb0, 0, NULL, &cb, 0, 0);
    m_cb_var->SetConstantBuffer(m_cb0);

    //////////////////////////////////////////////////////////////////////////
    // Set vertex buffer

    ID3D11Buffer *quad_vbs[] = {m_fullscreen_quad_vb};
    unsigned int stride = sizeof(fullscreen_quad_vertex);
    unsigned int offset = 0;
    d3d11_device_context->IASetVertexBuffers(0, 1, quad_vbs, &stride, &offset); 
    
    //////////////////////////////////////////////////////////////////////////
    // Render full-screen quad
    
    m_render_dof_tech->GetPassByIndex(0)->Apply(0, d3d11_device_context);
    d3d11_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    d3d11_device_context->Draw(4, 0);
    
    //////////////////////////////////////////////////////////////////////////
    // Unbound the resource

    m_color_rt_var->SetResource(0);
    m_depth_rt_var->SetResource(0);
    m_render_dof_tech->GetPassByIndex(0)->Apply(0, d3d11_device_context);
}
