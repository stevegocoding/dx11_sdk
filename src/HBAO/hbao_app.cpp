#include "hbao_app.h"

#include "hbao_scene.h"
#include "hbao_resolve.h"
#include "hbao_renderer.h"
#include "hbao_gui.h"

/*
hbao_app_ptr g_app;

//////////////////////////////////////////////////////////////////////////
// Application Parameters 
struct msaa_config_value
{
    msaa_config_value(std::wstring& _name, int _samples, int _quality)
        : name(_name), samples(_samples), quality(_quality) 
    {}
    std::wstring name;
    int samples; 
    int quality;
};

std::vector<msaa_config_value> g_msaa_mode_enum;
int g_current_msaa_mode = 0; 
bool g_half_res_ao = false; 

void init_app_params()
{
    using namespace boost::assign;
    g_msaa_mode_enum +=
        msaa_config_value(std::wstring(L"1x MSAA"),1,0),
        msaa_config_value(std::wstring(L"2x MSAA"),2,0),
        msaa_config_value(std::wstring(L"4x MSAA"),4,0),
        msaa_config_value(std::wstring(L"8x MSAA"),4,8),
        msaa_config_value(std::wstring(L"8xQ MSAA"),8,8),
        msaa_config_value(std::wstring(L"16x MSAA"),4,16),
        msaa_config_value(std::wstring(L"16xQ MSAA"),8,16);
}

c_hbao_application::c_hbao_application()
{
    init_app_params(); 

    m_hbao_scene.reset(new c_hbao_scene_component());
    m_hbao_resolve.reset(new c_hbao_resolve_component());
    m_hbao_engine.reset(new c_hbao_renderer_component(D3DX_PI / 4));
   
    m_hbao_gui.reset(new c_hbao_gui_component());
}

HRESULT c_hbao_application::on_create_resource(const render_sys_context_ptr& render_sys_context)
{
    c_demo_app_listener::on_create_resource(render_sys_context);
    
    m_hbao_scene->on_create_resource(render_sys_context);
    m_hbao_resolve->on_create_resource(render_sys_context); 
    m_hbao_engine->on_create_resource(render_sys_context); 
    m_hbao_gui->on_create_resource(render_sys_context);
    
    create_buffers();
    
    return S_OK;
}

void c_hbao_application::on_release_resource()
{
    release_buffers();

    m_hbao_gui->on_release_resource();
    m_hbao_engine->on_release_resource();
    m_hbao_resolve->on_release_resource();
    m_hbao_scene->on_release_resource();
}

void c_hbao_application::on_frame_render(double time, float elapsed_time)
{
    //////////////////////////////////////////////////////////////////////////
    // 1st: Render scene geometry to render texture
    d3d11_device_context_ptr device_context = m_render_sys_context->get_d3d11_device_context();
    
    ID3D11RenderTargetView *rts[] = {m_rt_gbuf_color->get_rtv(), m_rt_gbuf_normal->get_rtv()};
    float white_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    int num_rts = 2;

    device_context->ClearRenderTargetView(m_rt_gbuf_color->get_rtv(), white_color);
    device_context->ClearDepthStencilView(m_rt_gbuf_depth->get_dsv(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    device_context->OMSetRenderTargets(num_rts, rts, m_rt_gbuf_depth->get_dsv());

    m_hbao_scene->on_frame_render(time, elapsed_time);

    //////////////////////////////////////////////////////////////////////////
    // 2nd: Resolve color, normal and depth buffer to non-msaa render targets
    m_hbao_resolve->resolve_colors(m_rt_gbuf_color);
    m_hbao_resolve->resolve_depth(m_rt_gbuf_depth); 
    
    m_rt_resolved_color = m_hbao_resolve->get_resolved_color();
    m_rt_resolved_depth = m_hbao_resolve->get_resolved_depth();

    // m_hbao_engine->apply_hbao_nd(m_rt_resolved_depth, m_rt_resolved_normal);

    ////////////////////////////////////////////////////////////////////////
    // Finally: draw to screen
    d3d11_render_target_view_ptr backbuf_rtv = DXUTGetD3D11RenderTargetView();
    d3d11_depth_stencil_view_ptr backbuf_dsv = DXUTGetD3D11DepthStencilView();
    float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    ID3D11RenderTargetView *backbuf_rtvs[] = {backbuf_rtv};
    device_context->ClearRenderTargetView(backbuf_rtv, clear_color); 
    device_context->ClearDepthStencilView(backbuf_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    device_context->OMSetRenderTargets(1, backbuf_rtvs, backbuf_dsv);

    //////////////////////////////////////////////////////////////////////////
    // Render GUI
    m_hbao_gui->on_frame_render(time, elapsed_time);
}

void c_hbao_application::on_frame_update(double time, float elapsed_time)
{
    m_hbao_scene->on_frame_update(time, elapsed_time); 
}

HRESULT c_hbao_application::create_buffers()
{
    //////////////////////////////////////////////////////////////////////////
    // Create the render textures

    // Color texture resource with render target view
    D3D11_TEXTURE2D_DESC tex_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    memset(&tex_desc, 0, sizeof(D3D11_TEXTURE2D_DESC));
    memset(&srv_desc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    memset(&dsv_desc, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    
    unsigned int bb_width = m_render_sys_context->get_bbuf_desc()->Width; 
    unsigned int bb_height = m_render_sys_context->get_bbuf_desc()->Height;

    tex_desc.Width = bb_width;
    tex_desc.Height = bb_height;
    tex_desc.ArraySize = 1;
    tex_desc.MiscFlags = 0;
    tex_desc.MipLevels = 1;
    tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    tex_desc.SampleDesc.Count = g_msaa_mode_enum[g_current_msaa_mode].samples; 
    tex_desc.SampleDesc.Quality = g_msaa_mode_enum[g_current_msaa_mode].quality; 
    tex_desc.CPUAccessFlags = 0;
    tex_desc.Usage = D3D11_USAGE_DEFAULT;
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (g_msaa_mode_enum[g_current_msaa_mode].samples > 1)      // Multi-sampling
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    }
    else 
    {
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    }
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Texture2D.MostDetailedMip = 0; 

    m_rt_gbuf_color.reset(new c_texture2D(m_render_sys_context, &tex_desc, NULL));
    m_rt_gbuf_color->bind_rt_view(NULL);
    m_rt_gbuf_color->bind_sr_view(&srv_desc);

    // Normal texture resource with render target view and shader resource view
    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_rt_gbuf_normal.reset(new c_texture2D(m_render_sys_context, &tex_desc, NULL));
    m_rt_gbuf_normal->bind_rt_view(NULL); 
    m_rt_gbuf_normal->bind_sr_view(&srv_desc);

    // Depth texture resource with render target view and shader resource view
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    tex_desc.Format =  DXGI_FORMAT_R24G8_TYPELESS;
    dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.Texture2D.MipSlice = 0;
    dsv_desc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL; 
    srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    if (g_msaa_mode_enum[g_current_msaa_mode].samples > 1)
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    }
    else 
    {
        dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    }

    m_rt_gbuf_depth.reset(new c_texture2D(m_render_sys_context, &tex_desc, NULL));
    m_rt_gbuf_depth->bind_ds_view(&dsv_desc);
    m_rt_gbuf_depth->bind_sr_view(&srv_desc);
    return S_OK;
}

void c_hbao_application::release_buffers()
{
    m_rt_gbuf_color.reset(); 
    m_rt_gbuf_normal.reset();
    m_rt_gbuf_depth.reset();
}

void c_hbao_application::on_swap_chain_resized(const swap_chain_resize_event& event)
{
    m_hbao_scene->on_swap_chain_resized(event);
    
    m_hbao_resolve->set_input_rt_size(event.width, event.height);
    
    if (!g_half_res_ao)
        m_hbao_resolve->set_output_rt_size(event.width, event.height);
    else 
        m_hbao_resolve->set_output_rt_size(event.width/2, event.height/2);
    
    int num_samples = g_msaa_mode_enum[g_current_msaa_mode].samples; 
    m_hbao_resolve->set_num_samples(num_samples);

    m_hbao_resolve->on_swap_chain_resized(event); 
    
    // m_hbao_engine->on_swap_chain_resized(event);
    m_hbao_gui->on_swap_chain_resized(event);
}

void c_hbao_application::on_swap_chain_released()
{
    m_hbao_engine->on_swap_chain_released(); 
    m_hbao_gui->on_swap_chain_released(); 
}

LRESULT c_hbao_application::msg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    m_hbao_scene->msg_proc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing, pUserContext);
    m_hbao_gui->msg_proc(hWnd, uMsg, wParam, lParam, pbNoFurtherProcessing, pUserContext); 
    
    return 0;
}
*/