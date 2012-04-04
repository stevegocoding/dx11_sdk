#include "utils.h"

c_render_texture::c_render_texture(ID3D11Device *d3d11_device, D3D11_TEXTURE2D_DESC *tex_desc, D3D11_RENDER_TARGET_VIEW_DESC *rtv_desc /* = NULL */, D3D11_SHADER_RESOURCE_VIEW_DESC *srv_desc /* = NULL */, D3D11_DEPTH_STENCIL_VIEW_DESC *dsv_desc /* = NULL */)
{
    HRESULT hr;

    //These have to be set to have a render target
    if (srv_desc)
        tex_desc->BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (rtv_desc)
        tex_desc->BindFlags |= D3D11_BIND_RENDER_TARGET; 
    if (dsv_desc)
        tex_desc->BindFlags |= D3D11_BIND_DEPTH_STENCIL;

    tex_desc->Usage = D3D11_USAGE_DEFAULT;
    tex_desc->CPUAccessFlags = 0;

    V(d3d11_device->CreateTexture2D( tex_desc, NULL, &m_texture ));
    V(d3d11_device->CreateRenderTargetView(m_texture, rtv_desc, &m_rtv));

    if (srv_desc)
        V(d3d11_device->CreateShaderResourceView(m_texture, srv_desc, &m_srv));
    if (dsv_desc)
        V(d3d11_device->CreateDepthStencilView(m_texture, dsv_desc, &m_dsv)); 
}