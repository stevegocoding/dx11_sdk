#include "../pch.h"
#include "../render_system_context.h"
#include "../hbao_scene.h"
#include "../hbao_resolve.h"
#include "test_resolve.h"

static render_sys_context_ptr g_render_context;
static hbao_scene_ptr g_scene; 
static hbao_resolve_ptr g_resolver;

texture_2d_ptr g_gbuf_color_rtt; 
texture_2d_ptr g_gbuf_depth_rtt;

static void create_gbuf_rtts(int samples, int quality)
{
    D3D11_TEXTURE2D_DESC tex_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    memset(&tex_desc, 0, sizeof(D3D11_TEXTURE2D_DESC));
    memset(&srv_desc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    memset(&dsv_desc, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC)); 

    tex_desc.Width = g_render_context->get_bbuf_desc()->Width;
    tex_desc.Height = g_render_context->get_bbuf_desc()->Height;
    tex_desc.ArraySize = 1;
    tex_desc.MiscFlags = 0;
    tex_desc.MipLevels = 1;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.SampleDesc.Count = samples; 
    tex_desc.SampleDesc.Quality = quality; 
    tex_desc.CPUAccessFlags = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 

    if (samples > 1)
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;
        srv_desc.Texture2D.MostDetailedMip = 0; 
    }

    g_gbuf_color_rtt.reset(new c_texture2D(g_render_context, &tex_desc, NULL));
    g_gbuf_color_rtt->bind_rt_view(NULL); 
    g_gbuf_color_rtt->bind_sr_view(&srv_desc);

    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL; 
    tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS; 
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

    if (samples > 1)
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }
    else 
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; 
        dsv_desc.Texture2D.MipSlice = 0;
    }
    g_gbuf_depth_rtt.reset(new c_texture2D(g_render_context, &tex_desc, NULL));
    g_gbuf_depth_rtt->bind_ds_view(&dsv_desc);
    g_gbuf_depth_rtt->bind_sr_view(&srv_desc);
}

static HRESULT CALLBACK on_device_create( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
    void* pUserContext )
{
    g_render_context.reset(new c_render_system_context(DXUTGetD3D11Device(), 
        DXUTGetD3D11DeviceContext(), 
        DXUTGetDXGIBackBufferSurfaceDesc(), 
        DXUTGetDXGISwapChain())); 

    g_scene->on_create_resource(g_render_context); 
    g_resolver->on_create_resource(g_render_context);

    return S_OK;
}

static void CALLBACK on_device_destroy( void* pUserContext )
{
    g_scene->on_release_resource(); 
    g_resolver->on_release_resource();

    g_gbuf_color_rtt.reset(); 
    g_gbuf_depth_rtt.reset(); 

    g_scene.reset();
    g_resolver.reset(); 
    g_render_context.reset();
}

static HRESULT CALLBACK on_swap_chain_resize( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
    const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    create_gbuf_rtts(8, 0); 

    g_resolver->set_input_rt_size(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
    g_resolver->set_output_rt_size(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
    g_resolver->set_num_samples(8);
    g_resolver->set_z_near_far(1.0f, 200.0f); 

    g_resolver->on_swap_chain_resized(swap_chain_resize_event(
        pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height));

    return S_OK;
}

static void CALLBACK on_swap_chain_release( void* pUserContext )
{
    g_resolver->on_swap_chain_released();
}

static void CALLBACK on_frame_move( double fTime, float fElapsedTime, void* pUserContext )
{
    g_scene->on_frame_update(fTime, fElapsedTime);
}

static void CALLBACK on_frame_render( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
    double fTime, float fElapsedTime, void* pUserContext )
{
    PIX_EVENT_BEGIN_FRAME();

    ID3D11RenderTargetView *rtvs[] = {g_gbuf_color_rtt->get_rtv()};
    ID3D11DepthStencilView *dsv = g_gbuf_depth_rtt->get_dsv(); 
    float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    pd3dImmediateContext->ClearRenderTargetView(g_gbuf_color_rtt->get_rtv(), clear_color); 
    pd3dImmediateContext->ClearDepthStencilView(g_gbuf_depth_rtt->get_dsv(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    PIX_EVENT_BEGIN(Begin scene on_frame_render);
    
    PIX_EVENT_SET_RENDER_TARGETS();	
    pd3dImmediateContext->OMSetRenderTargets(1, rtvs, g_gbuf_depth_rtt->get_dsv());
    g_scene->on_frame_render(fTime, fElapsedTime);
    PIX_EVENT_END();

    //pd3dImmediateContext->OMSetRenderTargets(0, NULL, NULL); 

    /*
    ID3D11RenderTargetView *backbuf_rtv = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView *backbuf_dsv = DXUTGetD3D11DepthStencilView(); 

    pd3dImmediateContext->ClearRenderTargetView(backbuf_rtv, clear_color);
    pd3dImmediateContext->ClearDepthStencilView(backbuf_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    pd3dImmediateContext->OMSetRenderTargets(1, &backbuf_rtv, backbuf_dsv);
    */
    
    PIX_EVENT_BEGIN(Begin resolve resolve_colors);
    g_resolver->resolve_colors(g_gbuf_color_rtt); 
    PIX_EVENT_END();

    PIX_EVENT_BEGIN(Begin resolve resolve_nd); 
    g_resolver->resolve_depth(g_gbuf_depth_rtt); 
    PIX_EVENT_END();

    PIX_EVENT_END_FRAME()
}

static LRESULT CALLBACK msg_proc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
    g_scene->msg_proc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing, pUserContext);
    return 0;
}

void c_resolve_test_fixture::setup()
{
    g_scene.reset(new c_hbao_scene_component());
    g_resolver.reset(new c_hbao_resolve_component());

    setup_dxut_callbacks(on_device_create, 
        on_device_destroy,
        on_swap_chain_resize,
        on_swap_chain_release,
        on_frame_move,
        on_frame_render,
        msg_proc); 
    
    init_dxut(L"Resolve Test", true, 1024, 768);

    enter_main_loop();
}

void c_resolve_test_fixture::tear_down()
{
}

// TEST_F(c_resolve_test_fixture, resolver_test) {}