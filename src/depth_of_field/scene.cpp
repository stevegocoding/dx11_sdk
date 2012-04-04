#include "scene.h"
#include "prerequisites.h"

const std::wstring mesh_path_prefix = L"../data/MicroscopeCity/"; 
const std::wstring fx_file_name = L"scene.fx";
const std::string render_scene_tech_name = "render_scene"; 
const std::string cb_var_name = "cb0";
const std::string tex_diffuse_var_name = "g_tex_diffuse";

mesh_descriptor mesh_descs[] = 
{
    {L"city", mesh_path_prefix + std::wstring(L"occcity.sdkmesh")},
    {L"scanner", mesh_path_prefix + std::wstring(L"scanner.sdkmesh")},
    {L"column", mesh_path_prefix + std::wstring(L"column.sdkmesh")}
};

struct cb0
{
    XMMATRIX mat_wvp;
};

d3d11_resource_ptr g_resource;
std::vector<d3d11_texture2d_ptr> g_diffuse_tex_vec;

void CALLBACK load_texture_from_file(ID3D11Device *pdevice, char *file_name, ID3D11ShaderResourceView **ppsrv, void *user_context)
{
    HRESULT hr;
    
    d3d11_texture2d_ptr tex;
    std::string texture_path = std::string("../data/MicroscopeCity/") + std::string(file_name);
    D3DX11_IMAGE_LOAD_INFO img_load_info;
    D3DX11_IMAGE_INFO img_info;
    D3DX11GetImageInfoFromFileA(file_name, NULL, &img_info, NULL);
    img_load_info.pSrcInfo = &img_info; 
    V(D3DX11CreateTextureFromFileA(pdevice, texture_path.c_str(), &img_load_info, NULL, (ID3D11Resource**)&tex, &hr));
    
    // Create shader resource view for .fx 
    D3D11_TEXTURE2D_DESC tex_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc; 
    tex->GetDesc(&tex_desc);
    srv_desc.Format = tex_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    
    V(pdevice->CreateShaderResourceView(tex, &srv_desc, ppsrv));
}

c_app_scene::c_app_scene()
    : m_render_scene_tech(NULL)
    , m_cb0_var(NULL)
    , m_tex_diffuse(NULL)
{
    
}

HRESULT c_app_scene::on_d3d11_create_device(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc)
{
    HRESULT hr;

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
    
    m_render_scene_tech = m_scene_effect->GetTechniqueByName(render_scene_tech_name.c_str());
    m_cb0_var = m_scene_effect->GetConstantBufferByName(cb_var_name.c_str());
    m_tex_diffuse = m_scene_effect->GetVariableByName(tex_diffuse_var_name.c_str())->AsShaderResource();
    
    //////////////////////////////////////////////////////////////////////////
    // Create input layout

    const D3D11_INPUT_ELEMENT_DESC input_layout_mesh[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    unsigned int num_elements = ARRAYSIZE(input_layout_mesh);
    D3DX11_PASS_DESC pass_desc;
    m_render_scene_tech->GetPassByIndex(0)->GetDesc(&pass_desc);
    V(d3d11_device->CreateInputLayout(input_layout_mesh, num_elements, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &m_mesh_input_layout));
   
    //////////////////////////////////////////////////////////////////////////
    // Create constant buffer

    D3D11_BUFFER_DESC cb_desc;
    cb_desc.Usage =D3D11_USAGE_DEFAULT;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = 0; 
    cb_desc.MiscFlags = 0;
    cb_desc.ByteWidth = sizeof(cb0);
    V(d3d11_device->CreateBuffer(&cb_desc, NULL, &m_cb0)); 

    //////////////////////////////////////////////////////////////////////////
    // Load mesh

    m_city_mesh.reset(new CDXUTSDKMesh()); 
    m_scanner_mesh.reset(new CDXUTSDKMesh());
    m_column_mesh.reset(new CDXUTSDKMesh());
   
    SDKMESH_CALLBACKS11 sdkmesh_callbacks;
    sdkmesh_callbacks.pContext = this;
    sdkmesh_callbacks.pCreateIndexBuffer = 0; 
    sdkmesh_callbacks.pCreateVertexBuffer = 0; 
    sdkmesh_callbacks.pCreateTextureFromFile = load_texture_from_file; 
    V(m_city_mesh->Create(d3d11_device, mesh_descs[0].path.c_str(), true, &sdkmesh_callbacks));
    V(m_scanner_mesh->Create(d3d11_device, mesh_descs[1].path.c_str(), true, &sdkmesh_callbacks));
    V(m_column_mesh->Create(d3d11_device, mesh_descs[2].path.c_str(), true, &sdkmesh_callbacks));
    
    //////////////////////////////////////////////////////////////////////////
    // Setup the camera
    
    m_camera.reset(new CModelViewerCamera());
    XMVECTOR lookat = XMVectorSet(0.0f, 0.8f, -2.3f, 1.0f);
    XMVECTOR eye_pos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    m_camera->SetViewParams((D3DXVECTOR3*)&lookat, (D3DXVECTOR3*)&eye_pos);

    return S_OK;
}

void c_app_scene::on_d3d11_destroy_device()
{
    m_city_mesh->Destroy();
    m_scanner_mesh->Destroy();
    m_column_mesh->Destroy();
}

HRESULT c_app_scene::on_d3d11_resized_swap_chain(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc)
{
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
    XMMATRIX mat_inv_proj;
    XMMATRIX mat_wvp;
    
    mat_world = convert_d3dxmat_to_xnamat(*m_camera->GetWorldMatrix()); 
    mat_proj = convert_d3dxmat_to_xnamat(*m_camera->GetProjMatrix());
    mat_view = convert_d3dxmat_to_xnamat(*m_camera->GetViewMatrix());
    mat_wvp = mat_world * mat_view * mat_proj;
    XMVECTOR det_vec; 
    mat_inv_proj = XMMatrixInverse(&det_vec, mat_proj); 
    
    //////////////////////////////////////////////////////////////////////////
    // Set input layout
    d3d11_device_context->IASetInputLayout(m_mesh_input_layout);

    //////////////////////////////////////////////////////////////////////////
    // Update constant buffer
    cb0 cb;
    cb.mat_wvp = XMMatrixTranspose(mat_wvp);  
    d3d11_device_context->UpdateSubresource(m_cb0, 0, NULL, &cb, 0, 0);
    m_cb0_var->SetConstantBuffer(m_cb0);
    
    //////////////////////////////////////////////////////////////////////////
    // Render mesh
    m_render_scene_tech->GetPassByIndex(0)->Apply(0, d3d11_device_context); 
    m_city_mesh->Render(d3d11_device_context, 0); 
    m_column_mesh->Render(d3d11_device_context, 0); 
    
}

void c_app_scene::on_frame_update(double time, double elapsed_time)
{
    m_camera->FrameMove((float)elapsed_time); 
}

void c_app_scene::on_win_msg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    m_camera->HandleMessages(hWnd, uMsg, wParam, lParam); 
}