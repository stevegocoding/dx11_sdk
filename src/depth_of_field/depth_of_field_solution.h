#pragma once

#include "app_listener.h"

class c_depth_of_field_solution : public c_dx11_demo_app_listener
{
public:
    c_depth_of_field_solution(const modelview_camera_ptr& camera);

    virtual HRESULT on_d3d11_create_device(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc);
    virtual void on_d3d11_destroy_device();
    virtual HRESULT on_d3d11_resized_swap_chain(IDXGISwapChain *swap_chain, const DXGI_SURFACE_DESC *backbuf_surface_desc); 
    virtual void on_d3d11_releasing_swap_chain() {}; 
    virtual void render(ID3D11Device *d3d11_device, ID3D11DeviceContext *d3d11_device_context, const render_texture_ptr& color_rt, const render_texture_ptr& depth_rt, double time, float elapsed_time);
   
private:
    D3D11_VIEWPORT m_vp;
    
    d3d11_buffer_ptr m_cb0; 
    d3d11_buffer_ptr m_fullscreen_quad_vb; 
    d3dx11_effect_ptr m_dof_effect;
    
    float m_focal_plane;
    float m_screen_width, m_screen_height;
    
    modelview_camera_ptr m_camera;
   
    //////////////////////////////////////////////////////////////////////////
    // Effect Variables
    ID3DX11EffectShaderResourceVariable *m_color_rt_var;
    ID3DX11EffectShaderResourceVariable *m_depth_rt_var;
    ID3DX11EffectConstantBuffer *m_cb_var;
    ID3DX11EffectTechnique *m_render_dof_tech;
}; 

typedef boost::shared_ptr<c_depth_of_field_solution> dof_solu_ptr;