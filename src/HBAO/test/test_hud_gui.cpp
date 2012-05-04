#include "../pch.h"
#include "../render_system_context.h"
#include "../utils.h"

#include "test_hud_gui.h"
#include "../hbao_gui.h"

static render_sys_context_ptr g_render_context;
static hbao_gui_ptr g_gui; 

static HRESULT CALLBACK on_device_create( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
    void* pUserContext )
{	
    g_render_context.reset(new c_render_system_context(DXUTGetD3D11Device(), 
        DXUTGetD3D11DeviceContext(), 
        DXUTGetDXGIBackBufferSurfaceDesc(), 
        DXUTGetDXGISwapChain())); 

	g_gui->on_create_resource(g_render_context); 
	
    return S_OK;
}

static void CALLBACK on_device_destroy( void* pUserContext )
{
	g_gui->on_release_resource();

	g_gui.reset(); 
    g_render_context.reset();
}

static HRESULT CALLBACK on_swap_chain_resize( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
    const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	g_gui->on_swap_chain_resized(swap_chain_resize_event(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height)); 
    return S_OK;
}

static void CALLBACK on_swap_chain_release( void* pUserContext )
{
	g_gui->on_swap_chain_released(); 
}

static void CALLBACK on_frame_move( double fTime, float fElapsedTime, void* pUserContext )
{
	g_gui->on_frame_update(fTime, fElapsedTime); 
}

static void CALLBACK on_frame_render( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
    double fTime, float fElapsedTime, void* pUserContext )
{
    PIX_EVENT_BEGIN_FRAME();

	ID3D11RenderTargetView *backbuf_rtv = DXUTGetD3D11RenderTargetView();
	float clear_color[] = {0.0f, 0.0f, 0.0f, 0.0f}; 

	//pd3dImmediateContext->ClearRenderTargetView(backbuf_rtv, clear_color);

	g_gui->on_frame_render(fTime, fElapsedTime); 

	PIX_EVENT_END_FRAME(); 
}

static LRESULT CALLBACK msg_proc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
	g_gui->msg_proc(hWnd,uMsg,wParam,lParam,pbNoFurtherProcessing,pUserContext);
	
    return 0;
}

void c_hud_gui_test_fixture::setup()
{    
	g_gui.reset(new c_hbao_gui_component("hbao_demo.layout"));
	
    setup_dxut_callbacks(on_device_create, 
        on_device_destroy,
        on_swap_chain_resize,
        on_swap_chain_release,
        on_frame_move,
        on_frame_render,
        msg_proc); 
    
    init_dxut(L"GUI Test", true, 1024, 768);

    enter_main_loop();
}

void c_hud_gui_test_fixture::tear_down()
{
}
