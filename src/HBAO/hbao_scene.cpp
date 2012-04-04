#include "hbao_scene.h"

namespace 
{
    const std::wstring mesh_path_prefix = L"../data/"; 
    const std::wstring fx_file_name = L"scene.fx";

    enum e_models
    {
        RES_MESH_DRAGON = 0,
        RES_MESH_SIBNIKE, 
        MESH_COUNT
    };

    struct cb0
    {
        XMMATRIX mat_wvp;
        XMMATRIX mat_wv;
        XMMATRIX mat_wv_it;
    };
}

c_hbao_scene_component::c_hbao_scene_component()
    : m_current_model(0)
    , m_render_scene_tech(NULL)
    , m_param_cb0(NULL)
    , m_param_is_ground(NULL)
{

}

HRESULT c_hbao_scene_component::on_create_resource(const render_sys_context_ptr& render_sys_context)
{
    super::on_create_resource(render_sys_context);
    
    compile_effects(render_sys_context);
    setup_geometry(render_sys_context);
    setup_camera(render_sys_context);
    create_buffers(render_sys_context);

    return S_OK;
}

void c_hbao_scene_component::on_release_resource()
{
}

HRESULT c_hbao_scene_component::setup_geometry(const render_sys_context_ptr& render_sys_context)
{
    HRESULT hr;

    //////////////////////////////////////////////////////////////////////////
    // Create input layout
    const D3D11_INPUT_ELEMENT_DESC input_layout_mesh[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    unsigned int num_elements = ARRAYSIZE(input_layout_mesh);
    D3DX11_PASS_DESC pass_desc;
    m_render_scene_tech->GetPassByIndex(0)->GetDesc(&pass_desc);
    V(render_sys_context->get_d3d11_device()->CreateInputLayout(input_layout_mesh, num_elements, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &m_input_layout));
    
    //////////////////////////////////////////////////////////////////////////
    // Load mesh
    m_meshes.push_back(mesh_desc_ptr(new c_sdkmesh_wrapper(render_sys_context->get_d3d11_device(), L"dragon", mesh_path_prefix+std::wstring(L"dragon.sdkmesh"), true)));
    m_meshes.push_back(mesh_desc_ptr(new c_sdkmesh_wrapper(render_sys_context->get_d3d11_device(), L"sibenik", mesh_path_prefix+std::wstring(L"sibenik.sdkmesh"), false)));
    
    return S_OK;
}

HRESULT c_hbao_scene_component::compile_effects(const render_sys_context_ptr& render_sys_context)
{
    HRESULT hr; 
    
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
        render_sys_context->get_d3d11_device(),
        &m_scene_effect));
    
    // Build parameters table
    m_effect_params_table.reset(new c_d3dx11_effect_params_table(m_render_sys_context, m_scene_effect));
    m_render_scene_tech = m_effect_params_table->get_technique_by_name("render_scene_diffuse");
    m_param_cb0 = m_effect_params_table->get_variable_by_name("cb0")->AsConstantBuffer(); 
    m_param_is_ground = m_effect_params_table->get_variable_by_name("g_is_ground")->AsScalar(); 

    return S_OK;
}

HRESULT c_hbao_scene_component::create_buffers(const render_sys_context_ptr& render_sys_context)
{
    HRESULT hr;
    
    //////////////////////////////////////////////////////////////////////////
    // Create constant buffer
    D3D11_BUFFER_DESC buf_desc; 
    buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof(cb0);
    buf_desc.CPUAccessFlags = 0;
    buf_desc.MiscFlags = 0; 
    V(render_sys_context->get_d3d11_device()->CreateBuffer(&buf_desc, NULL, &m_cb0));

    return S_OK;
}

void c_hbao_scene_component::setup_camera(const render_sys_context_ptr& render_sys_context)
{
    unsigned int bbuf_width = render_sys_context->get_bbuf_width();
    unsigned int bbuf_height = render_sys_context->get_bbuf_height();

    // Setup the viewport
    m_vp.TopLeftX = 0;
    m_vp.TopLeftY = 0;
    m_vp.MinDepth = 0; 
    m_vp.MaxDepth = 1;
    m_vp.Width = (float)bbuf_width; 
    m_vp.Height = (float)bbuf_height;

    // Setup the camera projection
    float aspect_ratio = (float)bbuf_width / (float)bbuf_height;
   
    m_camera.reset(new CModelViewerCamera());
    XMVECTOR lookat = XMVectorSet(0.0f, 0.8f, -2.3f, 1.0f);
    XMVECTOR eye_pos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    
    m_camera->SetViewParams((D3DXVECTOR3*)&lookat, (D3DXVECTOR3*)&eye_pos);
    m_camera->SetProjParams( D3DX_PI / 4, aspect_ratio, 1.0f, 200.0f ); 
    m_camera->SetWindow( bbuf_width, bbuf_height ); 
    m_camera->SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
}

void c_hbao_scene_component::on_frame_render(double time, float elapsed_time)
{
    XMMATRIX mat_world;
    XMMATRIX mat_view;
    XMMATRIX mat_proj;
    mat_world = convert_d3dxmat_to_xnamat(*m_camera->GetWorldMatrix()); 
    mat_proj = convert_d3dxmat_to_xnamat(*m_camera->GetProjMatrix());
    mat_view = convert_d3dxmat_to_xnamat(*m_camera->GetViewMatrix());
    
    d3d11_device_context_ptr& device_context = m_render_sys_context->get_d3d11_device_context();

    device_context->RSSetViewports(1, &m_vp);

    update_transform_params(mat_world, mat_view, mat_proj);

    m_param_is_ground->SetBool(false);

    render_mesh();

    if (m_meshes[m_current_model]->with_ground)
    {
        XMMATRIX mat_translate;
        float h = (m_meshes[m_current_model]->bb_center.y) - (m_meshes[m_current_model]->bb_extents.y); 
        mat_translate = XMMatrixTranslation(0.0f, h, 0.0f);
        XMMATRIX world_tranform = mat_translate * mat_world; 
        update_transform_params(world_tranform, mat_view, mat_proj);

        m_param_is_ground->SetBool(true);

        render_ground_plane(); 
    }
}

void c_hbao_scene_component::on_frame_update(double time, float elapsed_time)
{
    if (m_camera)
        m_camera->FrameMove((float)elapsed_time); 
}

LRESULT c_hbao_scene_component::msg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    if (m_camera)
        m_camera->HandleMessages(hWnd, uMsg, wParam, lParam); 
    return 0;
}

void c_hbao_scene_component::render_mesh()
{
    d3d11_device_context_ptr device_context = m_render_sys_context->get_d3d11_device_context();

    if (m_current_model >= 0 && m_current_model < (int)m_meshes.size())
    {
        device_context->IASetInputLayout(m_input_layout);
        m_render_scene_tech->GetPassByIndex(0)->Apply(0, device_context); 
        m_meshes[m_current_model]->mesh_ptr->Render(device_context); 
    }
}

void c_hbao_scene_component::render_ground_plane()
{
    d3d11_device_context_ptr device_context = m_render_sys_context->get_d3d11_device_context();

    unsigned int stride = sizeof(ground_plane_vertex);
    unsigned int offset = 0;

    ID3D11Buffer *vbs[] = {m_meshes[m_current_model]->plane_vb};
    device_context->IASetVertexBuffers(0, 1, vbs, &stride, &offset); 
    device_context->IASetIndexBuffer(m_meshes[m_current_model]->plane_ib, DXGI_FORMAT_R32_UINT, 0);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context->IASetInputLayout(m_input_layout);

    m_render_scene_tech->GetPassByIndex(0)->Apply(0, device_context);
    device_context->DrawIndexed(6, 0, 0);
}

void c_hbao_scene_component::update_transform_params(const XMMATRIX& mat_world, const XMMATRIX& mat_view, const XMMATRIX& mat_proj)
{
    d3d11_device_context_ptr device_context = m_render_sys_context->get_d3d11_device_context();

    XMMATRIX mat_wvp = mat_world * mat_view * mat_proj;
    XMMATRIX mat_wv = mat_world * mat_view;
    XMVECTOR det;

    cb0 cb;
    cb.mat_wvp = XMMatrixTranspose(mat_wvp);
    cb.mat_wv = XMMatrixTranspose(mat_wv); 
    cb.mat_wv_it = XMMatrixTranspose(XMMatrixTranspose(XMMatrixInverse(&det, mat_wv))); 

    device_context->UpdateSubresource(m_cb0, 0, NULL, &cb, 0, 0);
    m_param_cb0->SetConstantBuffer(m_cb0);
}