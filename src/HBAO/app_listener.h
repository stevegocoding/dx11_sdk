#pragma once

#include "prerequisites.h"
#include "app_event.h"
#include "render_system_context.h"
#include "utils.h"

class c_dx11_demo_app_listener
{
public:
    virtual HRESULT on_create_device11(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc) { return S_OK; }
    virtual void on_destroy_device11() {};
    virtual HRESULT on_resized_swap_chain11(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc) { return S_OK; }
    virtual void on_releasing_swap_chain11() {}
    virtual void on_frame_render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, double time, float elapsed_time) {}
    virtual void on_frame_update(double time, double elapsed_time) {}
    virtual void on_win_msg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext) {}
};

class c_demo_app_listener
{
public:
    virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context)
    {
        m_render_sys_context = render_sys_context;
        return S_OK;
    }
    virtual void on_release_resource() {}
    virtual void on_frame_render(double time, float elapsed_time) {}
    virtual void on_frame_update(double time, float elapsed_time) {}
    virtual LRESULT msg_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext) { return 0; }
    
    virtual void on_swap_chain_resized(const swap_chain_resize_event& event) {}
    virtual void on_swap_chain_released() {}
    
    virtual ~c_demo_app_listener() {}
    
protected:
    render_sys_context_ptr m_render_sys_context;
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
class c_app_config_value
{
public:
    typedef boost::shared_ptr<c_app_config_value<T> > config_value_ptr; 
        
    c_app_config_value(T& val)
        : m_value(val)
    {}
    
    const T& get_val() const { return m_value; }

private:
    T m_value;
};

template <typename T> 
typename c_app_config_value<T>::config_value_ptr 
    make_config_value(T& val)
{
    return typename c_app_config_value<T>::config_value_ptr(new c_app_config_value<T>(val));
}

template <typename T, typename dxut_control_t>
class c_app_config_list
{
public:
    typedef boost::shared_ptr<c_app_config_list<T, dxut_control_t> > app_config_list_ptr;
    typedef std::vector<typename c_app_config_value<T>::config_value_ptr> name_value_vector;
    typedef typename name_value_vector::iterator name_value_it;
    
    const typename c_app_config_value<T>::config_value_ptr 
        get_value(unsigned int i) const 
    {
        assert(i < m_name_values.size());
        return m_name_values[i];
    }

    virtual void bind(dxut_control_t *control)
    {
    }
    
protected:
    name_value_vector m_name_values;
};
