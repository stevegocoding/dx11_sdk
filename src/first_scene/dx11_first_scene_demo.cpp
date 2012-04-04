
#include "dx11_first_scene_demo.h" 

d3d11_device_ptr g_d3d11_deivce;
d3d11_device_context_ptr g_immediate_device_context;

d3d11_input_layout_ptr g_mesh_input_layout;
d3d11_input_layout_ptr g_quad_input_layout;

d3d11_buffer_ptr g_vertex_buffer;
d3d11_buffer_ptr g_index_buffer;

d3d11_render_target_view_ptr g_render_target_view; 

d3dx11_effect_ptr g_effect;
ID3DX11EffectMatrixVariable *g_mat_wvp_var = NULL; 
ID3DX11EffectTechnique *g_technique = NULL;

CModelViewerCamera g_camera; 

WCHAR *phong_shading_fx_file_name = L"default.fx";
//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
    DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D11 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
// - Shaders
// - Buffers
// - Input layouts
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
    void* pUserContext )
{
    g_d3d11_deivce.Attach(pd3dDevice);
    g_d3d11_deivce->GetImmediateContext(&g_immediate_device_context); 
    
    HRESULT hr;
   
    // Create effect
    d3d11_blob_ptr effect_blob, error_blob;
    V(D3DX11CompileFromFile(phong_shading_fx_file_name, NULL, NULL, NULL, "fx_5_0", D3D10_SHADER_DEBUG, NULL, NULL, &effect_blob, &error_blob, NULL));
    V(D3DX11CreateEffectFromMemory(
        effect_blob->GetBufferPointer(),
        effect_blob->GetBufferSize(),
        0,
        g_d3d11_deivce,
        &g_effect));
    
    g_mat_wvp_var = g_effect->GetVariableByName("g_mat_wvp")->AsMatrix();
    g_technique = g_effect->GetTechniqueByName("render_scene");
    
    // Define input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    D3DX11_PASS_DESC pass_desc;
    hr = g_technique->GetPassByIndex(0)->GetDesc(&pass_desc);

    unsigned int num_elements = ARRAYSIZE(layout);
    V(g_d3d11_deivce->CreateInputLayout(layout, num_elements, pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &g_mesh_input_layout));
    g_immediate_device_context->IASetInputLayout(g_mesh_input_layout); 

    // Create and bind vertex buffers
    simple_vertex vertices[] = 
    {
        simple_vertex(XMFLOAT3(-5, 5, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f)),
        simple_vertex(XMFLOAT3(5, 5, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f)),
        simple_vertex(XMFLOAT3(5, -5, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f)),
        simple_vertex(XMFLOAT3(-5, -5, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f))
    };
    D3D11_BUFFER_DESC buf_desc;
    ZeroMemory(&buf_desc, sizeof(D3D11_BUFFER_DESC));
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof(simple_vertex) * 4;
    buf_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER; 
    buf_desc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA init_data;
    ZeroMemory(&init_data, sizeof(D3D11_SUBRESOURCE_DATA));
    init_data.pSysMem = vertices; 
    init_data.SysMemPitch = 0;
    init_data.SysMemSlicePitch = 0;
    V(g_d3d11_deivce->CreateBuffer(&buf_desc, &init_data, &g_vertex_buffer));

    // Create and bind index buffer
    unsigned int indices[6] = {0, 1, 2, 0, 2, 3};
    ZeroMemory(&buf_desc, sizeof(D3D11_BUFFER_DESC)); 
    buf_desc.Usage = D3D11_USAGE_DEFAULT;
    buf_desc.ByteWidth = sizeof(unsigned int) * 6;
    buf_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buf_desc.CPUAccessFlags = 0;
    buf_desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA index_data; 
    ZeroMemory(&index_data, sizeof(D3D11_SUBRESOURCE_DATA));
    index_data.pSysMem = indices; 
    V(g_d3d11_deivce->CreateBuffer(&buf_desc, &index_data, &g_index_buffer));
    
    //Get the mesh
    //IA setup
    unsigned int vb_stride = sizeof(simple_vertex);
    unsigned int vb_offset = 0; 
    ID3D11Buffer *p_bufs[1]; 
    p_bufs[0] = g_vertex_buffer;
    g_immediate_device_context->IASetVertexBuffers(0, 1, p_bufs, &vb_stride, &vb_offset); 
    g_immediate_device_context->IASetIndexBuffer(g_index_buffer, DXGI_FORMAT_R32_UINT, 0);
    g_immediate_device_context->IASetInputLayout( g_mesh_input_layout );

    // Set primitive topology
    g_immediate_device_context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    
    // Setup the camera
    D3DXVECTOR3 lookat( 0.0f, 0.0f, 1.0f );
    D3DXVECTOR3 eye_pos( 0.0f, 0.0f, 0.0f );
    g_camera.SetViewParams( &eye_pos, &lookat );

    D3DXVECTOR3 vCenter( 0.25767413f, -28.503521f, 111.00689f );
    FLOAT fObjectRadius = 378.15607f;
    //g_camera.SetRadius( fObjectRadius * 3.0f, fObjectRadius * 0.5f, fObjectRadius * 10.0f );
    
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
// - Render target textures
// - Viewport
// - 
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
    const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr; 

    // Create render target view
    d3d11_texture2d_ptr back_buf;
    V(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buf));
    V(pd3dDevice->CreateRenderTargetView(back_buf, NULL, &g_render_target_view));
    ID3D11RenderTargetView *rt_views[1];
    rt_views[0] = g_render_target_view;
    g_immediate_device_context->OMSetRenderTargets(1, rt_views, NULL);
   
    // Setup the viewport
    RECT rect = DXUTGetWindowClientRect();
    unsigned int width = rect.right - rect.left;
    unsigned int height = rect.top - rect.bottom;
    D3D11_VIEWPORT vp[1];
    vp[0].Width = 640.0f;
    vp[0].Height = 480.0f;
    vp[0].MinDepth = 0.0f;
    vp[0].MaxDepth = 1.0f;
    vp[0].TopLeftX = 0;
    vp[0].TopLeftY = 0;
    g_immediate_device_context->RSSetViewports( 1, vp );
    
    // Setup the camera projection
    float fAspectRatio = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_camera.SetProjParams( D3DX_PI / 4, fAspectRatio, 1.0f, 200.0f );
    g_camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_camera.SetButtonMasks( MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_LEFT_BUTTON );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    // Update the camera's position based on user input 
    g_camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
    double fTime, float fElapsedTime, void* pUserContext )
{
    // Clear render target and the depth stencil 
    float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 1.0f };
    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    g_immediate_device_context->ClearRenderTargetView( pRTV, ClearColor );
    g_immediate_device_context->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    D3DXMATRIX mat_world;
    D3DXMATRIX mat_view;
    D3DXMATRIX mat_proj;
    D3DXMATRIX mat_wvp;

    D3DXMatrixTranslation(&mat_world, 0.0f, 0.0f, 15.0f);
    mat_proj = *g_camera.GetProjMatrix();
    mat_view = *g_camera.GetViewMatrix();
    mat_wvp = mat_world * mat_view * mat_proj; 
    
    g_mat_wvp_var->SetMatrix(mat_wvp); 
    
    // Apply the technique to update state.
    g_technique->GetPassByIndex(0)->Apply(0, pd3dImmediateContext);

    // Render a triangle
    g_immediate_device_context->DrawIndexed( 6, 0, 0 );
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_render_target_view = NULL;
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_mesh_input_layout = NULL;
    g_vertex_buffer = NULL;
    g_index_buffer = NULL; 

    g_effect = NULL;

    g_immediate_device_context = NULL; 
    g_d3d11_deivce.Detach(); 
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Pass all remaining windows messages to camera so it can respond to user input
    g_camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
    bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
    int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D11) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    // Perform any application-level initialization here

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"EmptyProject11" );

    // Only require 10-level hardware
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT ren  der loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}


