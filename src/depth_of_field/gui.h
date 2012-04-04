#pragma once

#include "pch.h"
#include "prerequisites.h"

#include "CEGUISystem.h"
#include "CEGUIDefaultResourceProvider.h"
#include "CEGUIImageset.h"
#include "CEGUIFont.h"
#include "CEGUIScheme.h"
#include "CEGUIWindowManager.h"
#include "CEGUIFontManager.h"
#include "RendererModules/Direct3D11/CEGUIDirect3D11Renderer.h"
#include "falagard/CEGUIFalWidgetLookManager.h"
#include "CEGUIScriptModule.h"
#include "CEGUIXMLParser.h"
#include "CEGUIAnimationManager.h"

void init_gui_system(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context);
void destroy_gui_system();
void render_gui();

void on_gui_mouse_move(float x, float y);
void on_gui_mouse_button_down(CEGUI::MouseButton button);
void on_gui_mouse_button_up(CEGUI::MouseButton button); 
void on_gui_mouse_wheel_change(float delta);

typedef boost::function<bool(const CEGUI::EventArgs&)> event_handler_func;
class c_gui_main_window
{
public:
    c_gui_main_window(const std::string& layout_name, const bool make_default = true);
    ~c_gui_main_window(); 
    
    void subscribe_gui_handler(const std::string& wnd_name, event_handler_func handler);
};
typedef boost::shared_ptr<c_gui_main_window> main_wnd_ptr;

