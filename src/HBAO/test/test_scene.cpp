#include "../pch.h"
#include "../render_system_context.h"
#include "../hbao_scene.h"
#include "test_scene.h"

render_sys_context_ptr g_render_context;
hbao_scene_ptr g_scene; 

static HRESULT CALLBACK on_device_create( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
    void* pUserContext )
{
    g_render_context.reset(new 
        c_render_system_context(DXUTGetD3D11Device(), 
        DXUTGetD3D11DeviceContext(), 
        DXUTGetDXGIBackBufferSurfaceDesc(), 
        DXUTGetDXGISwapChain())); 

    g_scene->on_create_resource(g_render_context); 

    return S_OK;
}

static void CALLBACK on_device_destroy( void* pUserContext )
{
    g_scene.reset();
    g_render_context.reset();
}

static HRESULT CALLBACK on_swap_chain_resize( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
    const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{

    return S_OK;
}

static void CALLBACK on_swap_chain_release( void* pUserContext )
{
}

static void CALLBACK on_frame_move( double fTime, float fElapsedTime, void* pUserContext )
{
    g_scene->on_frame_update(fTime, fElapsedTime);
}

static void CALLBACK on_frame_render( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
    double fTime, float fElapsedTime, void* pUserContext )
{   
    ID3D11RenderTargetView *backbuf_rtv = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView *backbuf_dsv = DXUTGetD3D11DepthStencilView(); 
    float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f}; 

    pd3dImmediateContext->ClearRenderTargetView(backbuf_rtv, clear_color);
    pd3dImmediateContext->ClearDepthStencilView(backbuf_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    pd3dImmediateContext->OMSetRenderTargets(1, &backbuf_rtv, backbuf_dsv);

    g_scene->on_frame_render(fTime, fElapsedTime);

}

static LRESULT CALLBACK msg_proc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
    g_scene->msg_proc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing, pUserContext);
    return 0;
}

void c_scene_test_fixture::setup()
{
    g_scene.reset(new c_hbao_scene_component());

    setup_dxut_callbacks(on_device_create, 
        on_device_destroy,
        on_swap_chain_resize,
        on_swap_chain_release,
        on_frame_move,
        on_frame_render,
        msg_proc); 

    init_dxut(L"Scene Test", true, 800, 800);

    enter_main_loop();
}

void c_scene_test_fixture::tear_down()
{
}