#pragma once

#include "pch.h"
#include "prerequisites.h"

inline XMMATRIX convert_d3dxmat_to_xnamat(const D3DXMATRIX& d3dx_mat)
{
    return XMMatrixSet( d3dx_mat._11, d3dx_mat._12, d3dx_mat._13, d3dx_mat._14,
                        d3dx_mat._21, d3dx_mat._22, d3dx_mat._23, d3dx_mat._24,
                        d3dx_mat._31, d3dx_mat._32, d3dx_mat._33, d3dx_mat._34,
                        d3dx_mat._41, d3dx_mat._42, d3dx_mat._43, d3dx_mat._44);
}

class c_render_texture
{
public:
    c_render_texture(ID3D11Device *d3d11_device, D3D11_TEXTURE2D_DESC *tex_desc, D3D11_RENDER_TARGET_VIEW_DESC *rtv_desc = NULL, D3D11_SHADER_RESOURCE_VIEW_DESC *srv_desc = NULL, D3D11_DEPTH_STENCIL_VIEW_DESC *dsv_desc = NULL);

    d3d11_texture2d_ptr m_texture;
        d3d11_render_target_view_ptr m_rtv;
        d3d11_shader_resourse_view_ptr m_srv;
        d3d11_depth_stencil_view_ptr m_dsv;
};
typedef boost::shared_ptr<c_render_texture> render_texture_ptr;