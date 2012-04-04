#pragma once

#include "prerequisites.h"
#include "app_listener.h"
#include "utils.h" 

class c_hbao_resolve_component : public c_demo_app_listener
{
    typedef c_demo_app_listener super;

public:
    virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context);
    virtual void on_release_resource(); 
    virtual void on_swap_chain_resized(const swap_chain_resize_event& event);
    
    void resolve_colors(const texture_2d_ptr& input_color); 
    void resolve_depth(const texture_2d_ptr& input_depth);
    
    void set_input_rt_size(int width, int height);  
    void set_output_rt_size(int width, int height); 
    void set_num_samples(int num_samples) { m_num_samples = num_samples; }
    void set_z_near_far(float z_near, float z_far) {m_z_near = z_near, m_z_far = z_far; }
    
    texture_2d_ptr& get_resolved_color() { return m_rt_resolved_color; }
    texture_2d_ptr& get_resolved_depth() { return m_rt_resolved_depth; }

private:
    HRESULT recreate_render_textures();
    HRESULT recompile_effect();
    void apply_resolve_depth(const texture_2d_ptr& input_depth);
    void apply_linearize_depth(const texture_2d_ptr& input_depth, texture_2d_ptr& output_depth);
    void update_cb_proj_planes();

    //////////////////////////////////////////////////////////////////////////
    // Resources
    texture_2d_ptr m_rt_resolved_color;
    texture_2d_ptr m_rt_resolved_depth;
    texture_2d_ptr m_src_rt_depth;
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
    // Cached effect parameters
    d3dx11_effect_ptr m_resolve_effect;
    effect_params_table_ptr m_effect_params_table;
    
    ID3DX11EffectTechnique *m_resolve_depth_tech, *m_linearize_depth_tech; 
    ID3DX11EffectShaderResourceVariable *m_tex_depth_sr;
    ID3DX11EffectConstantBuffer *m_proj_plane_cb;
};

typedef boost::shared_ptr<c_hbao_resolve_component> hbao_resolve_ptr; 