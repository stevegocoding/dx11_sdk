
#include "scene.h"
#include "depth_of_field_solution.h"

//////////////////////////////////////////////////////////////////////////
// Render Textures
render_texture_ptr g_rt_color;
render_texture_ptr g_rt_depth;

scene_ptr g_scene;
dof_solu_ptr g_dof_solution; 

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
    g_scene.reset(new c_app_scene());
    
    if (g_scene)
    {
        g_scene->on_d3d11_create_device(pd3dDevice, pBackBufferSurfaceDesc); 
        g_dof_solution.reset(new c_depth_of_field_solution(g_scene->get_camera()));
        g_dof_solution->on_d3d11_create_device(pd3dDevice, pBackBufferSurfaceDesc); 
    }
    
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
    // Create the render target texture and its resource view (render target view and shader resource view)
    unsigned int width = pBackBufferSurfaceDesc->Width;
    unsigned int height = pBackBufferSurfaceDesc->Height;

    D3D11_TEXTURE2D_DESC tex_desc;
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 0;
    tex_desc.ArraySize = 1;
    tex_desc.Format = pBackBufferSurfaceDesc->Format;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
    D3D11_RENDER_TARGET_VIEW_DESC rt_view_desc;
    rt_view_desc.Format = pBackBufferSurfaceDesc->Format;
    rt_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rt_view_desc.Texture2D.MipSlice = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC sr_view_desc;
    sr_view_desc.Format = pBackBufferSurfaceDesc->Format;
    sr_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sr_view_desc.Texture2D.MostDetailedMip = 0;
    sr_view_desc.Texture2D.MipLevels = 6;
    g_rt_color.reset(new c_render_texture(pd3dDevice, &tex_desc, &rt_view_desc, &sr_view_desc, NULL));

    // Create the depth-stencil texture and its resource view (depth stencil view)
    ::ZeroMemory(&tex_desc, sizeof(D3D11_TEXTURE2D_DESC));
    tex_desc.Width = width;
    tex_desc.Height = height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    D3D11_DEPTH_STENCIL_VIEW_DESC ds_view_desc;
    ZeroMemory(&ds_view_desc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    ds_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ds_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    ds_view_desc.Texture2D.MipSlice = 0;
    ds_view_desc.Flags = 0;
    
    ZeroMemory(&sr_view_desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    sr_view_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    sr_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sr_view_desc.Texture2D.MipLevels = 1;
    sr_view_desc.Texture2D.MostDetailedMip = 0;
    
    g_rt_depth.reset(new c_render_texture(pd3dDevice, &tex_desc, NULL, &sr_view_desc, &ds_view_desc)); 
    
    g_scene->on_d3d11_resized_swap_chain(pSwapChain, pBackBufferSurfaceDesc);
    g_dof_solution->on_d3d11_resized_swap_chain(pSwapChain, pBackBufferSurfaceDesc);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    g_scene->on_frame_update(fTime, fElapsedTime);
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
    double fTime, float fElapsedTime, void* pUserContext )
{   
    d3d11_render_target_view_ptr backbuf_rtv = DXUTGetD3D11RenderTargetView();
    d3d11_depth_stencil_view_ptr backbuf_dsv = DXUTGetD3D11DepthStencilView();
    float clear_color[] = {0.9569f, 0.9569f, 1.0f, 0.0f};
    ID3D11RenderTargetView *rtvs[] = {g_rt_color->m_rtv};
    ID3D11RenderTargetView *backbuf_rtvs[] = {backbuf_rtv};
    
    pd3dImmediateContext->ClearRenderTargetView(g_rt_color->m_rtv, clear_color);
    pd3dImmediateContext->ClearDepthStencilView(g_rt_depth->m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    pd3dImmediateContext->OMSetRenderTargets(1, rtvs, g_rt_depth->m_dsv);

    g_scene->on_frame_render(pd3dDevice, pd3dImmediateContext, fTime, fElapsedTime);

    pd3dImmediateContext->ClearRenderTargetView(backbuf_rtv, clear_color); 
    pd3dImmediateContext->ClearDepthStencilView(backbuf_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    pd3dImmediateContext->OMSetRenderTargets(1, backbuf_rtvs, backbuf_dsv);

    g_dof_solution->render(pd3dDevice, pd3dImmediateContext, g_rt_color, g_rt_depth, fTime, fElapsedTime); 
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
    g_rt_color.reset();
    g_rt_depth.reset();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_dof_solution.reset();
    g_scene.reset();
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    bool* pbNoFurtherProcessing, void* pUserContext )
{
    if (g_scene)
        g_scene->get_camera()->HandleMessages(hWnd, uMsg, wParam, lParam);

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
    DXUTCreateWindow( L"DOF Demo" );

    // Only require 10-level hardware
    DXUTCreateDevice( D3D_FEATURE_LEVEL_11_0, true, 640, 480 );
    DXUTMainLoop(); // Enter into the DXUT ren  der loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}