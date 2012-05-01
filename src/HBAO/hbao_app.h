#pragma once
#include "app_listener.h"

class c_hbao_scene_component;
class c_hbao_resolve_component;
class c_hbao_renderer_component;
class c_hbao_gui_component;

typedef boost::shared_ptr<c_hbao_scene_component> hbao_scene_ptr;
typedef boost::shared_ptr<c_hbao_resolve_component> hbao_resolve_ptr;
typedef boost::shared_ptr<c_hbao_renderer_component> hbao_engine_ptr; 
typedef boost::shared_ptr<c_hbao_gui_component> hbao_gui_ptr;

/*
class c_hbao_application : public c_demo_app_listener
{
public:
    c_hbao_application();
    virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context);
    virtual void on_release_resource();

    virtual void on_frame_render(double time, float elapsed_time);
    virtual void on_frame_update(double time, float elapsed_time);
    virtual void on_swap_chain_resized(const swap_chain_resize_event& event);
    virtual void on_swap_chain_released();
    virtual LRESULT msg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext);
    
private:
    void init_gui();
    HRESULT create_buffers();

    
    //HRESULT create_gbuf_color_rtt(); 
    //HRESULT create_gbuf_normal_rtt(); 
    //HRESULT create_gbuf_depth_rtt(); 
    

    void release_buffers(); 
   
    //////////////////////////////////////////////////////////////////////////
    // Components
    hbao_scene_ptr m_hbao_scene;
    hbao_resolve_ptr m_hbao_resolve;
    hbao_engine_ptr m_hbao_engine;
    hbao_gui_ptr m_hbao_gui;
    
    //////////////////////////////////////////////////////////////////////////
    // Resources
    texture_2d_ptr m_rt_gbuf_color;
    texture_2d_ptr m_rt_gbuf_normal;
    texture_2d_ptr m_rt_gbuf_depth;
    texture_2d_ptr m_rt_resolved_color;
    texture_2d_ptr m_rt_resolved_normal;
    texture_2d_ptr m_rt_resolved_depth;
};

typedef boost::shared_ptr<c_hbao_application> hbao_app_ptr; 
extern hbao_app_ptr g_app;

*/