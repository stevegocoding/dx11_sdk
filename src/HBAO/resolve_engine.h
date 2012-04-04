#pragma once
#include "app_listener.h" 

typedef boost::shared_ptr<class c_render_texture> render_texture_ptr;

class c_resolve_engine : public c_dx11_demo_app_listener
{
public:
    virtual HRESULT on_create_device11(ID3D11Device *d3d11_device, const DXGI_SURFACE_DESC *backbuf_surface_desc);
    virtual void on_destroy_device11();
    HRESULT on_params_changed(ID3D11DeviceContext *device_context, int in_width, int in_height, int num_samples, 
                              int out_width, int out_height, 
                              float z_near, float z_far);

    
    HRESULT resolve_color(ID3D11DeviceContext *device_context, const render_texture_ptr& rt_color, render_texture_ptr& resolved_rt_color);
    HRESULT resolve_normal_depth(ID3D11DeviceContext *device_context,
                                 const render_texture_ptr& src_rt_normal, 
                                 const render_texture_ptr& src_rt_depth, 
                                 render_texture_ptr& resolved_rt_normal,
                                 render_texture_ptr& resolved_rt_depth);
    
private:
    d3d11_shader_resourse_view_ptr& linearize_depth(ID3D11DeviceContext *device_context, d3d11_shader_resourse_view_ptr& src_depth_srv);
    void resolve_normal_depth_impl(ID3D11DeviceContext *device_context, ID3D11ShaderResourceView *linear_depth, ID3D11ShaderResourceView *normal); 
    HRESULT recompile_effect();

private:
    //////////////////////////////////////////////////////////////////////////
    // Resources
    d3d11_device_ptr m_device;
    render_texture_ptr m_rt_resolved_color;
    render_texture_ptr m_rt_resolved_normal; 
    render_texture_ptr m_rt_resolved_depth;
    render_texture_ptr m_src_rt_depth;
    d3d11_buffer_ptr m_cb_proj_planes;
    
    //////////////////////////////////////////////////////////////////////////
    // Parameters
    int m_in_width, m_in_height;
    int m_out_width, m_out_height;
    float m_z_near, m_z_far;
    int m_num_samples;
    bool m_need_upsample; 
    D3D11_VIEWPORT m_in_vp, m_out_vp;
    
    //////////////////////////////////////////////////////////////////////////
    // Effects 
    d3dx11_effect_ptr m_resolve_effect; 
    effect_tech_vector m_tech_vec;
    effect_cb_param_vector m_cb_params;
    effect_sr_param_vector m_sr_params;
    
};
typedef boost::shared_ptr<c_resolve_engine> resolve_engine_ptr;