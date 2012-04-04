#include "render_system_context.h"

c_render_system_context::c_render_system_context(ID3D11Device *device, ID3D11DeviceContext *device_context, const DXGI_SURFACE_DESC *bbuf_surface_desc, IDXGISwapChain *swap_chain)
    : m_device(device)
    , m_device_context(device_context)
    , m_bbuf_surface_desc(bbuf_surface_desc)
    , m_swap_chain(swap_chain)
{
}
c_render_system_context::~c_render_system_context()
{
    m_device.Release(); 
    m_device_context.Release();
    m_swap_chain.Release();
}

void c_render_system_context::on_resized_swap_chain(IDXGISwapChain* pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
    m_swap_chain = pSwapChain;
    m_bbuf_surface_desc = pBackBufferSurfaceDesc;
}