#include "hbao_gui.h"

namespace
{
    float radius_mul_default = 1.0f;
    float angle_bias_default = 30;
    int num_dirs_default = 16; 
    int num_steps_default = 8; 
    float contrast_default = 1.25f;
    float att_default = 1.0f;

    const std::string cegui_data_path_prefix = "../data/CEGUI/"; 
}

c_hbao_gui_component::c_hbao_gui_component(const std::string& layout_name)
    : m_d3d11_renderer(NULL)
    , m_mouse_in_wnd(false)
	, m_layout_file_name(layout_name)
	, m_manual_setup_ui(layout_name.empty())
{
	
}

HRESULT c_hbao_gui_component::on_create_resource(const render_sys_context_ptr& render_sys_context)
{
    HRESULT hr;
    hr = super::on_create_resource(render_sys_context); 
    
    D3D11_VIEWPORT vp;
    vp.Width = (float)(render_sys_context->get_bbuf_width());
    vp.Height = (float)(render_sys_context->get_bbuf_height()); 
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_render_sys_context->get_d3d11_device_context()->RSSetViewports(1, &vp);
    
    init_cegui_renderer(m_render_sys_context); 
    init_cegui_resouce();
    
    m_default_wnd.reset(new c_gui_main_window(m_layout_file_name, true));
    
    return hr; 
}

void c_hbao_gui_component::setup_ui()
{
	using namespace CEGUI;
	
	WindowManager& win_mgr = WindowManager::getSingleton(); 
 
	
}

void c_hbao_gui_component::on_release_resource()
{
    CEGUI::Direct3D11Renderer::destroySystem(); 
}

void c_hbao_gui_component::on_frame_render(double time, float elapsed_time)
{
    CEGUI::System::getSingleton().renderGUI();
}

void c_hbao_gui_component::on_frame_update(double time, float elapsed_time)
{
}

void c_hbao_gui_component::on_swap_chain_resized(const swap_chain_resize_event& event)
{
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Size((float)event.width, (float)event.height));
}

void c_hbao_gui_component::on_swap_chain_released()
{
}

void c_hbao_gui_component::init_cegui_resouce()
{
    // initialise the required dirs for the DefaultResourceProvider
    CEGUI::DefaultResourceProvider* rp =
        static_cast<CEGUI::DefaultResourceProvider*>
        (CEGUI::System::getSingleton().getResourceProvider());

    CEGUI::String resource_path;
    // for each resource type, set a resource group directory
    resource_path = cegui_data_path_prefix + "schemes/";
    rp->setResourceGroupDirectory("schemes", resource_path);
    resource_path = cegui_data_path_prefix + "imagesets/";
    rp->setResourceGroupDirectory("imagesets", resource_path);
    resource_path = cegui_data_path_prefix + "fonts/";
    rp->setResourceGroupDirectory("fonts", resource_path);
    resource_path = cegui_data_path_prefix + "layouts/";
    rp->setResourceGroupDirectory("layouts", resource_path);
    resource_path = cegui_data_path_prefix + "looknfeel/";
    rp->setResourceGroupDirectory("looknfeels", resource_path);
    resource_path = cegui_data_path_prefix + "lua_scripts/";
    rp->setResourceGroupDirectory("lua_scripts", resource_path);
    resource_path = cegui_data_path_prefix + "xml_schemas/";
    rp->setResourceGroupDirectory("schemas", resource_path);

    // set the default resource groups to be used
    CEGUI::Imageset::setDefaultResourceGroup("imagesets");
    CEGUI::Font::setDefaultResourceGroup("fonts");
    CEGUI::Scheme::setDefaultResourceGroup("schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
    CEGUI::WindowManager::setDefaultResourceGroup("layouts");
    CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
    CEGUI::AnimationManager::setDefaultResourceGroup("animations");

    // setup default group for validation schemas
    CEGUI::XMLParser* parser = CEGUI::System::getSingleton().getXMLParser();
    if (parser->isPropertyPresent("SchemaDefaultResourceGroup"))
        parser->setProperty("SchemaDefaultResourceGroup", "schemas");  
    
    CEGUI::SchemeManager::getSingleton().create("OgreTray.scheme");
    CEGUI::SchemeManager::getSingleton().create("TaharezLook.scheme");
    CEGUI::SchemeManager::getSingleton().create("VanillaSkin.scheme");
    CEGUI::FontManager::getSingleton().create("DejaVuSans-7.font");
    CEGUI::System::getSingleton().setDefaultFont( "DejaVuSans-7" );
    CEGUI::System::getSingleton().setDefaultMouseCursor("TaharezLook", "MouseArrow");
}

void c_hbao_gui_component::mouse_enters(void)
{
    if (!m_mouse_in_wnd)
    {
        m_mouse_in_wnd = true;
        ShowCursor(false);
    }
}

void c_hbao_gui_component::mouse_leaves(void)
{
    if (m_mouse_in_wnd)
    {
        m_mouse_in_wnd = false;
        ShowCursor(true);
    }
}

void c_hbao_gui_component::init_cegui_renderer( render_sys_context_ptr& render_context )
{
    m_d3d11_renderer = &CEGUI::Direct3D11Renderer::bootstrapSystem(render_context->get_d3d11_device(), render_context->get_d3d11_device_context());
}

LRESULT c_hbao_gui_component::msg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    switch(uMsg)
    {
    case WM_CHAR:
        CEGUI::System::getSingleton().injectChar((CEGUI::utf32)wParam);
        break;
        
    case WM_SIZE:
        ///
        break;

    case WM_MOUSELEAVE:
        mouse_leaves();
        break;

    case WM_NCMOUSEMOVE:
        mouse_leaves();
        break;

    case WM_MOUSEMOVE:
        mouse_enters();

        CEGUI::System::getSingleton().injectMousePosition((float)(LOWORD(lParam)), (float)(HIWORD(lParam)));
        break;

    case WM_LBUTTONDOWN:
        CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::LeftButton);
        break;

    case WM_LBUTTONUP:
        CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::LeftButton);
        break;

    case WM_RBUTTONDOWN:
        CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::RightButton);
        break;

    case WM_RBUTTONUP:
        CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::RightButton);
        break;

    case WM_MBUTTONDOWN:
        CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::MiddleButton);
        break;

    case WM_MBUTTONUP:
        CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::MiddleButton);
        break;

    case 0x020A: // WM_MOUSEWHEEL:
        CEGUI::System::getSingleton().injectMouseWheelChange(static_cast<float>((short)HIWORD(wParam)) / static_cast<float>(120));
        break;
    }
    return 0; 
}

//////////////////////////////////////////////////////////////////////////

c_gui_main_window::c_gui_main_window(const std::string& layout_name, const bool make_default)
{
    using namespace CEGUI;

    WindowManager& wnd_mgr = WindowManager::getSingleton(); 
    Window *sheet = wnd_mgr.loadWindowLayout(layout_name); 

    if (make_default)
        System::getSingleton().setGUISheet(sheet);
}

c_gui_main_window::~c_gui_main_window()
{

}

void c_gui_main_window::subscribe_gui_handler(const std::string& wnd_name, event_handler_func handler)
{
    using namespace CEGUI;

    Window *wnd = WindowManager::getSingleton().getWindow(wnd_name);
    assert(wnd);

    wnd->subscribeEvent(wnd_name, handler);
}