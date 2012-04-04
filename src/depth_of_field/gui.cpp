
#include "gui.h"



namespace
{
    static const std::string cegui_data_path_prefix = "../data/cegui_data/"; 
}

void init_default_resource_group(); 
void init_resource_group_directory(); 
void init_basic_resource();
void init_renderer(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context);

void init_gui_system(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context); 
void destroy_gui_system();
void render_gui();

void on_gui_mouse_move(float x, float y)
{
    CEGUI::System::getSingleton().injectMousePosition(x, y);
}
void on_gui_mouse_button_down(CEGUI::MouseButton button)
{
    CEGUI::System::getSingleton().injectMouseButtonClick(button); 
}
void on_gui_mouse_button_up(CEGUI::MouseButton button)
{
    CEGUI::System::getSingleton().injectMouseButtonUp(button);
}
void on_gui_mouse_wheel_change(float delta)
{
    CEGUI::System::getSingleton().injectMouseWheelChange(delta);
}

void init_gui_system(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context)
{
    init_renderer(d3d11_device, d3d11_device_context);
    init_resource_group_directory();
    init_default_resource_group();
    init_basic_resource(); 
}

void destroy_gui_system()
{
    CEGUI::Direct3D11Renderer::destroySystem(); 
}

void render_gui()
{
    CEGUI::System::getSingleton().renderGUI();
}

void init_resource_group_directory()
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
}

void init_default_resource_group()
{
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
}

void init_basic_resource()
{
    CEGUI::SchemeManager::getSingleton().create("TaharezLook.scheme");
    CEGUI::FontManager::getSingleton().create("DejaVuSans-9.font");
    CEGUI::System::getSingleton().setDefaultFont( "DejaVuSans-9" );
    CEGUI::System::getSingleton().setDefaultMouseCursor("TaharezLook", "MouseArrow");
}

void init_renderer(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context)
{
    try {
        CEGUI::Direct3D11Renderer& renderer = CEGUI::Direct3D11Renderer::bootstrapSystem(d3d11_device, d3d11_device_context);
    }
    catch(CEGUI::RendererException& e) 
    {
        int a = 0; 
    }
}

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