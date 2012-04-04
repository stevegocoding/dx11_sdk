#pragma once 
#include "prerequisites.h"

class c_render_system_context;
typedef boost::shared_ptr<c_render_system_context> render_sys_context_ptr;

class c_render_system_context
{
public:
    c_render_system_context(ID3D11Device *device, ID3D11DeviceContext *device_context, const DXGI_SURFACE_DESC *bbuf_surface_desc, IDXGISwapChain *swap_chain); 
    ~c_render_system_context();
    
    d3d11_device_ptr& get_d3d11_device() { return m_device; }
    d3d11_device_context_ptr& get_d3d11_device_context() { return m_device_context; }
    unsigned int get_bbuf_width() const { return m_bbuf_surface_desc->Width; }
    unsigned int get_bbuf_height() const { return m_bbuf_surface_desc->Height; }
    const DXGI_SURFACE_DESC* get_bbuf_desc() const { return m_bbuf_surface_desc; }
    dxgi_swap_chain_ptr& get_dxgi_swap_chain() { return m_swap_chain; }
    
    void on_resized_swap_chain(IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc); 

private:

    d3d11_device_ptr m_device; 
    d3d11_device_context_ptr m_device_context;
    const DXGI_SURFACE_DESC *m_bbuf_surface_desc;
    dxgi_swap_chain_ptr m_swap_chain;
};