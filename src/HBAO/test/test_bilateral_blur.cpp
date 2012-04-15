#include "../pch.h"
#include "../render_system_context.h"
#include "../hbao_scene.h"
#include "../hbao_resolve.h"
#include "../hbao_bilateral_blur.h"
#include "../hbao_renderer.h"

#include "test_bilateral_blur.h"

#define NUM_MSAA_SAMPLES 8
#define FOVY (40.0f * D3DX_PI / 180.0f)
#define ZNEAR 0.1f
#define ZFAR 500.0f

// ---------------------------------------------------------------------
/*
	 Global Variables
*/ 
// ---------------------------------------------------------------------
static render_sys_context_ptr g_render_context;
static hbao_scene_ptr g_scene; 
static hbao_resolve_ptr g_resolver;
static hbao_component_ptr g_hbao_renderer;
static hbao_bilateral_blur_ptr g_bilateral_blur; 

static texture_2d_ptr g_gbuf_color_rtt; 
static texture_2d_ptr g_gbuf_depth_rtt;

blur_app_params g_blur_app_params;
hbao_app_params g_hbao_params;

// ---------------------------------------------------------------------
/*
	Global Functions 
*/ 
// ---------------------------------------------------------------------

void init_blur_app_params()
{
	g_blur_app_params.blur_radius = 10; 
	g_blur_app_params.sharpness = 0; 
	g_blur_app_params.edge_threshold = 0;
}

void init_hbao_params()
{
	g_hbao_params.radius = 1.0f; 
	g_hbao_params.step_size = 4; 
	g_hbao_params.angle_bias = 10.0f;
	g_hbao_params.stength = 1.0f;
	g_hbao_params.power_exponent = 1.0f;
	g_hbao_params.blur_radius = 16; 
	g_hbao_params.blur_sharpness = 8.0f; 
	g_hbao_params.m_ao_res_mode = e_full_res_ao;
}


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
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	}
	else 
	{
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D; 
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		dsv_desc.Texture2D.MipSlice = 0;
	}
	g_gbuf_depth_rtt.reset(new c_texture2D(g_render_context, &tex_desc, NULL));
	g_gbuf_depth_rtt->bind_ds_view(&dsv_desc);
	g_gbuf_depth_rtt->bind_sr_view(&srv_desc);
}


static HRESULT CALLBACK on_device_create( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext )
{	
	init_blur_app_params(); 
	init_hbao_params();
	
	g_render_context.reset(new c_render_system_context(DXUTGetD3D11Device(), 
		DXUTGetD3D11DeviceContext(), 
		DXUTGetDXGIBackBufferSurfaceDesc(), 
		DXUTGetDXGISwapChain())); 

	g_scene->on_create_resource(g_render_context); 
	g_resolver->on_create_resource(g_render_context);
	g_hbao_renderer->on_create_resource(g_render_context); 
	g_bilateral_blur->on_create_resource(g_render_context);
	
	// Initialize the ao parameters 
	g_hbao_renderer->set_ao_parameters(g_hbao_params);
	
	
	return S_OK; 
}

static void CALLBACK on_device_destroy( void* pUserContext )
{
	g_scene->on_release_resource(); 
	g_resolver->on_release_resource();
	g_bilateral_blur->on_release_resource(); 

	g_gbuf_color_rtt.reset(); 
	g_gbuf_depth_rtt.reset(); 

	g_scene.reset();
	g_resolver.reset();
	g_bilateral_blur.reset(); 
	g_hbao_renderer.reset();
	
	g_render_context.reset();
}

static HRESULT CALLBACK on_swap_chain_resize( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	unsigned int bbuf_width = pBackBufferSurfaceDesc->Width; 
	unsigned int bbuf_height = pBackBufferSurfaceDesc->Height; 

	create_gbuf_rtts(NUM_MSAA_SAMPLES, 0);

	g_resolver->set_input_rt_size(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_resolver->set_output_rt_size(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
	g_resolver->set_num_samples(8);
	g_resolver->set_z_near_far(1.0f, 200.0f); 
	g_resolver->on_swap_chain_resized(swap_chain_resize_event(
		pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height));
	
	g_bilateral_blur->set_num_samples(NUM_MSAA_SAMPLES); 
	
	g_bilateral_blur->set_viewport(bbuf_width, bbuf_height); 
	
	g_bilateral_blur->set_resolution_params(bbuf_width, bbuf_height, bbuf_width, bbuf_height); 
	
	g_bilateral_blur->create_sr_textures();

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

	//////////////////////////////////////////////////////////////////////////
	// Render the scene to gbuffer (color and depth)
	//////////////////////////////////////////////////////////////////////////

	float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
	pd3dImmediateContext->ClearRenderTargetView(g_gbuf_color_rtt->get_rtv(), clear_color); 
	pd3dImmediateContext->ClearDepthStencilView(g_gbuf_depth_rtt->get_dsv(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView *rtvs[] = {g_gbuf_color_rtt->get_rtv()};
	ID3D11DepthStencilView *dsv = g_gbuf_depth_rtt->get_dsv(); 


	//////////////////////////////////////////////////////////////////////////
	// Start rendering the scene
	//////////////////////////////////////////////////////////////////////////
	PIX_EVENT_BEGIN(Begin scene rendering);

	pd3dImmediateContext->OMSetRenderTargets(1, rtvs, g_gbuf_depth_rtt->get_dsv());
	g_scene->on_frame_render(fTime, fElapsedTime); 
	
	PIX_EVENT_END(); 

	////////////////////////////////////////////////////////////////////////// 
	// Resolve the depth
	//////////////////////////////////////////////////////////////////////////
	PIX_EVENT_BEGIN(Begin resolve resolve_colors);
	g_resolver->resolve_colors(g_gbuf_color_rtt); 
	PIX_EVENT_END();

	PIX_EVENT_BEGIN(Begin resolve resolve_nd); 
	g_resolver->resolve_depth(g_gbuf_depth_rtt); 
	PIX_EVENT_END();


	//////////////////////////////////////////////////////////////////////////
	// Start bilateral blur
	//////////////////////////////////////////////////////////////////////////
	PIX_EVENT_BEGIN(Begin bilateral blur);
	
	ID3D11RenderTargetView *bbuf_rtv = DXUTGetD3D11RenderTargetView();

	g_bilateral_blur->set_dest_rtv(bbuf_rtv); 
	g_bilateral_blur->set_blur_params(g_blur_app_params); 
	g_bilateral_blur->set_resources(g_resolver->get_resolved_color(), g_resolver->get_resolved_color(), g_resolver->get_resolved_depth(), g_gbuf_depth_rtt); 
	g_bilateral_blur->on_frame_render(fTime, fElapsedTime); 
	// g_bilateral_blur->apply_passthrough(bbuf_rtv, g_resolver->get_resolved_color()); 
	
	PIX_EVENT_END();

	PIX_EVENT_END_FRAME(); 

}

static LRESULT CALLBACK msg_proc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{ 
	g_scene->msg_proc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing, pUserContext);
	return 0;
}

void c_bilateral_blur_test_fixture::setup()
{
	g_scene.reset(new c_hbao_scene_component());
	g_resolver.reset(new c_hbao_resolve_component());
	g_hbao_renderer.reset(new c_hbao_renderer_component(FOVY));
	g_bilateral_blur.reset(new c_bilateral_blur());

	setup_dxut_callbacks(on_device_create, 
		on_device_destroy,
		on_swap_chain_resize,
		on_swap_chain_release,
		on_frame_move,
		on_frame_render,
		msg_proc); 

	init_dxut(L"Bliteral Blur Test", true, 1024, 768);

	enter_main_loop();
}

void c_bilateral_blur_test_fixture::tear_down()
{

}