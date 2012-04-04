#include "scene.h"
#include "prerequisites.h"




c_app_scene::c_app_scene()
    : m_current_model(0)
{
    
}

HRESULT c_app_scene::on_create_device11(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc)
{
    HRESULT hr;
    
    init_effect_params();

    ////////////////////////////////////////////////////////////////////////
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
        &m_scene_effect));
    
    //////////////////////////////////////////////////////////////////////////
    // Get the effect variables
    std::back_insert_iterator<effect_tech_vector> tech_ins = std::back_inserter(m_tech_vec);
    std::back_insert_iterator<effect_cb_param_vector>  cb_ins = std::back_inserter(m_cb_param_vec);
    for (int i = 0; i < (int)tech_names.size(); ++i, ++tech_ins)
        *tech_ins = m_scene_effect->GetTechniqueByName(tech_names[i].c_str());
    for (int i = 0; i < (int)cb_names.size(); ++i, ++cb_ins)
        *cb_ins = m_scene_effect->GetConstantBufferByName(cb_names[i].c_str());    
    
    //////////////////////////////////////////////////////////////////////////
    // Create input layout
    const D3D11_INPUT_ELEMENT_DESC input_layout_mesh[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    unsigned int num_elements = ARRAYSIZE(input_layout_mesh);
    D3DX11_PASS_DESC pass_desc;
    m_tech_vec[RENDER_SCENE_DIFFUSE]->GetPassByIndex(0)->GetDesc(&pass_desc);
    V(d3d11_device->CreateInputLayout(input_layout_mesh, num_elements, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &m_input_layout));
   
    //////////////////////////////////////////////////////////////////////////
    // Create constant buffer
    d3d11_buffer_ptr temp_buf1, temp_buf2;
    D3D11_BUFFER_DESC buf_desc; 
    buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof(cb0);
    buf_desc.CPUAccessFlags = 0;
    buf_desc.MiscFlags = 0; 
    V(d3d11_device->CreateBuffer(&buf_desc, NULL, &temp_buf1));
    m_res_bufs.push_back(temp_buf1);
    
    ZeroMemory(&buf_desc,sizeof(D3D11_BUFFER_DESC));
    buf_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof(cb_global);
    buf_desc.CPUAccessFlags = 0;
    buf_desc.MiscFlags = 0; 
    V(d3d11_device->CreateBuffer(&buf_desc, NULL, &temp_buf2));
    m_res_bufs.push_back(temp_buf2);

    //////////////////////////////////////////////////////////////////////////
    // Load mesh
    m_meshes.push_back(mesh_desc_ptr(new c_sdkmesh_wrapper(d3d11_device, L"dragon", mesh_path_prefix+std::wstring(L"dragon.sdkmesh"), true)));
    m_meshes.push_back(mesh_desc_ptr(new c_sdkmesh_wrapper(d3d11_device, L"sibenik", mesh_path_prefix+std::wstring(L"sibenik.sdkmesh"), false)));
    
    //////////////////////////////////////////////////////////////////////////
    // Setup the camera
    m_camera.reset(new CModelViewerCamera());
    XMVECTOR lookat = XMVectorSet(0.0f, 0.8f, -2.3f, 1.0f);
    XMVECTOR eye_pos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    m_camera->SetViewParams((D3DXVECTOR3*)&lookat, (D3DXVECTOR3*)&eye_pos);

    return S_OK;
}

void c_app_scene::on_destroy_device11()
{
}

HRESULT c_app_scene::on_resized_swap_chain11(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc)
{
    m_backbuf_width = backbuf_surface_desc->Width;
    m_backbuf_height = backbuf_surface_desc->Height;
    
    // Setup the viewport
    m_vp.TopLeftX = 0;
    m_vp.TopLeftY = 0;
    m_vp.MinDepth = 0; 
    m_vp.MaxDepth = 1;
    m_vp.Width = (float)m_backbuf_width; 
    m_vp.Height = (float)m_backbuf_height;

    // Setup the camera projection
    float aspect_ratio = backbuf_surface_desc->Width / ( FLOAT )backbuf_surface_desc->Height;
    m_camera->SetProjParams( D3DX_PI / 4, aspect_ratio, 1.0f, 200.0f ); 
    
    m_camera->SetWindow( backbuf_surface_desc->Width, backbuf_surface_desc->Height ); 
    m_camera->SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );
    
    return S_OK; 
}

void c_app_scene::on_frame_render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, double time, float elapsed_time)
{
    XMMATRIX mat_world;
    XMMATRIX mat_view;
    XMMATRIX mat_proj;
    
    mat_world = convert_d3dxmat_to_xnamat(*m_camera->GetWorldMatrix()); 
    mat_proj = convert_d3dxmat_to_xnamat(*m_camera->GetProjMatrix());
    mat_view = convert_d3dxmat_to_xnamat(*m_camera->GetViewMatrix());
    
    d3d11_device_context->RSSetViewports(1, &m_vp);

    update_transform_params(d3d11_device_context, mat_world, mat_view, mat_proj);
   
    render_mesh(d3d11_device_context);
    
    if (m_meshes[m_current_model]->with_ground)
    {
        XMMATRIX mat_translate;
        float h = (m_meshes[m_current_model]->bb_center.y) - (m_meshes[m_current_model]->bb_extents.y); 
        mat_translate = XMMatrixTranslation(0.0f, h, 0.0f);
        XMMATRIX world_tranform = mat_translate * mat_world; 
        update_transform_params(d3d11_device_context, world_tranform, mat_view, mat_proj);
        render_ground_plane(d3d11_device_context); 
    }
}

void c_app_scene::on_frame_update(double time, double elapsed_time)
{
    m_camera->FrameMove((float)elapsed_time); 
}

void c_app_scene::on_win_msg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    m_camera->HandleMessages(hWnd, uMsg, wParam, lParam); 
}

void c_app_scene::render_mesh(ID3D11DeviceContext *device_context)
{
    if (m_current_model >= 0 && m_current_model < (int)m_meshes.size())
    {
        device_context->IASetInputLayout(m_input_layout);
        m_tech_vec[RENDER_SCENE_DIFFUSE]->GetPassByIndex(0)->Apply(0, device_context); 
        m_meshes[m_current_model]->mesh_ptr->Render(device_context); 
    }
}

void c_app_scene::render_ground_plane(ID3D11DeviceContext *device_context)
{
    unsigned int stride = sizeof(ground_plane_vertex);
    unsigned int offset = 0;
    
    ID3D11Buffer *vbs[] = {m_meshes[m_current_model]->plane_vb};
    device_context->IASetVertexBuffers(0, 1, vbs, &stride, &offset); 
    device_context->IASetIndexBuffer(m_meshes[m_current_model]->plane_ib, DXGI_FORMAT_R32_UINT, 0);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context->IASetInputLayout(m_input_layout);
   
    m_tech_vec[RENDER_SCENE_DIFFUSE]->GetPassByIndex(0)->Apply(0, device_context);
    device_context->DrawIndexed(6, 0, 0);
}

void c_app_scene::update_transform_params(ID3D11DeviceContext *device_context, const XMMATRIX& mat_world, const XMMATRIX& mat_view, const XMMATRIX& mat_proj)
{
    XMMATRIX mat_wvp = mat_world * mat_view * mat_proj;
    XMMATRIX mat_wv = mat_world * mat_view;
    XMVECTOR det;
    
    cb0 cb;
    cb.mat_wvp = XMMatrixTranspose(mat_wvp);
    cb.mat_wv = XMMatrixTranspose(mat_wv); 
    cb.mat_wv_it = XMMatrixTranspose(XMMatrixTranspose(XMMatrixInverse(&det, mat_wv))); 
    
    device_context->UpdateSubresource(m_res_bufs[RES_CB0], 0, NULL, &cb, 0, 0);
    m_cb_param_vec[CB0]->SetConstantBuffer(m_res_bufs[RES_CB0]);
}

void c_app_scene::update_global_params(ID3D11DeviceContext *device_context, const bool is_ground_plane)
{
    cb_global cb;
    cb.is_ground_plane = is_ground_plane; 
    device_context->UpdateSubresource(m_res_bufs[RES_CB_GLOBAL], 0, NULL, &cb, 0, 0);
    m_cb_param_vec[CB_GLOBAL]->SetConstantBuffer(m_res_bufs[RES_CB_GLOBAL]);
}