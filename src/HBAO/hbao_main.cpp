#include "hbao_app.h"
#include "test/test_scene.h"
#include "test/test_resolve.h"
#include "test/test_hbao.h"

//render_sys_context_ptr g_render_sys_context;

//HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
//    void* pUserContext )
//{
//    g_render_sys_context.reset(new 
//        c_render_system_context(DXUTGetD3D11Device(), 
//                                DXUTGetD3D11DeviceContext(), 
//                                DXUTGetDXGIBackBufferSurfaceDesc(), 
//                                DXUTGetDXGISwapChain())); 
//
//    // g_app.reset(new c_hbao_application());
//    // g_app->on_create_resource(g_render_sys_context); 
//    
//    
//    return S_OK;
//}
//
//HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
//    const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
//{
//    g_render_sys_context->on_resized_swap_chain(pSwapChain, pBackBufferSurfaceDesc);
//    //g_app->on_swap_chain_resized(swap_chain_resize_event(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height));
//
//    return S_OK;
//}
//
//void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
//{
//   // g_app->on_frame_update(fTime, fElapsedTime);
//}
//
//void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
//    double fTime, float fElapsedTime, void* pUserContext )
//{   
//    //g_app->on_frame_render(fTime, fElapsedTime); 
//}
//
//void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
//{
//    //g_app->on_swap_chain_released();
//}
//
//void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
//{
//   // g_app->on_release_resource();
//   // g_app.reset();
//    g_render_sys_context.reset(); 
//}
//
//LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
//    bool* pbNoFurtherProcessing, void* pUserContext )
//{
//    //if (g_app)
//    //    g_app->msg_proc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing, pUserContext);
//
//    return 0;
//}

c_scene_test_fixture g_scene_test; 
c_resolve_test_fixture g_resolve_test;
c_hbao_test_fixture g_hbao_test;

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    g_hbao_test.setup();   
    g_hbao_test.tear_down(); 

    return DXUTGetExitCode();
}