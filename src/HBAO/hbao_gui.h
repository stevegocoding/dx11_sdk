#pragma once
#include "app_listener.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif 

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

/*
struct msaa_config_value
{
    msaa_config_value(std::wstring& _name, int _samples, int _quality)
        : name(_name), samples(_samples), quality(_quality) 
    {}
    std::wstring name;
    int samples; 
    int quality;
};

struct mesh_config_value
{
    mesh_config_value(std::wstring& _name, std::wstring& _file_name)
        : name(_name)
        , file_name(_file_name)
    {}
    std::wstring name;
    std::wstring file_name;
};

class c_hbao_msaa_config : public c_app_config_list<msaa_config_value, CDXUTComboBox>
{
public:
    c_hbao_msaa_config()
    {
        using namespace boost::assign;
        m_name_values +=
            make_config_value(msaa_config_value(std::wstring(L"1x MSAA"),1,0)),
            make_config_value(msaa_config_value(std::wstring(L"2x MSAA"),2,0)),
            make_config_value(msaa_config_value(std::wstring(L"4x MSAA"),4,0)),
            make_config_value(msaa_config_value(std::wstring(L"8x MSAA"),4,8)),
            make_config_value(msaa_config_value(std::wstring(L"8xQ MSAA"),8,8)),
            make_config_value(msaa_config_value(std::wstring(L"16x MSAA"),4,16)),
            make_config_value(msaa_config_value(std::wstring(L"16xQ MSAA"),8,16));
    }

    virtual void bind(CDXUTComboBox *combobox)
    {
        int i = 0;
        for (name_value_it it = m_name_values.begin(); it != m_name_values.end(); ++it, ++i)
        {
            combobox->AddItem((*it)->get_val().name.c_str(), (void*)i);
        }
    }
};

class c_hbao_mesh_config : public c_app_config_list<mesh_config_value, CDXUTComboBox>
{
public:
    c_hbao_mesh_config()
    {
        using namespace boost::assign;
        m_name_values +=
            make_config_value(mesh_config_value(std::wstring(L"Dragon"), std::wstring(L"dragon.sdkmesh"))),
            make_config_value(mesh_config_value(std::wstring(L"Sibenik"), std::wstring(L"sibenik.sdkmesh")));
    }
    virtual void bind(CDXUTComboBox *combobox)
    {
        int i = 0;
        for (name_value_it it = m_name_values.begin(); it != m_name_values.end(); ++it, ++i)
        {
            combobox->AddItem((*it)->get_val().name.c_str(), (void*)i);
        }
    }
};
*/

typedef boost::function<bool(const CEGUI::EventArgs&)> event_handler_func;
class c_gui_main_window
{
public:
    c_gui_main_window(const std::string& layout_name, const bool make_default = true);
    ~c_gui_main_window(); 

    void subscribe_gui_handler(const std::string& wnd_name, event_handler_func handler);
};
typedef boost::shared_ptr<c_gui_main_window> main_wnd_ptr;

class c_hbao_gui_component : public c_demo_app_listener
{
    typedef c_demo_app_listener super;

public:
    explicit c_hbao_gui_component(const std::string& layout_name = std::string());
    virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context);
    virtual void on_release_resource(); 
    virtual void on_frame_render(double time, float elapsed_time); 
    virtual void on_frame_update(double time, float elapsed_time);
    virtual LRESULT msg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);

    // Application Events
    virtual void on_swap_chain_resized(const swap_chain_resize_event& event);
    virtual void on_swap_chain_released(); 

	virtual void setup_ui();

private:
    void mouse_enters();
    void mouse_leaves();

    void init_cegui_resouce();
    void init_cegui_renderer(render_sys_context_ptr& render_context);

    CEGUI::Direct3D11Renderer *m_d3d11_renderer;
    main_wnd_ptr m_default_wnd;
    bool m_mouse_in_wnd;

	std::string m_layout_file_name;
	
	bool m_manual_setup_ui; 
};
typedef boost::shared_ptr<c_hbao_gui_component> hbao_gui_ptr; 