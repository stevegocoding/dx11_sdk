#pragma once

#include "app_listener.h"

//class c_hbao_engine_component : public c_demo_app_listener
//{
//    typedef c_demo_app_listener super;
//public:
//    c_hbao_engine_component(float fovy);
//
//    virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context); 
//    virtual void on_swap_chain_resized(const swap_chain_resize_event& event);
//
//    void apply_hbao_nd(const render_texture_ptr& input_depth, const render_texture_ptr& input_normal); 
//    
//    void init_hbao_params(hbao_params_ptr& ao_params) {}
//    
//
//private:
//    void compile_shaders();
//    void update_rt(unsigned int bbuf_w, unsigned int bbuf_h);
//    void update_viewport(unsigned int bbuf_w, unsigned int bbuf_h); 
//
//    void init_params(); 
//    void update_num_steps(); 
//    void update_angle_biase(); 
//    void update_num_dirs(); 
//    void update_radius(); 
//    void update_attenuation();
//    void update_contrast();
//    void update_focal_len(unsigned int bbuf_w, unsigned int bbuf_h); 
//    void update_resolution(unsigned int bbuf_w, unsigned int bbuf_h);
//    void update_aspect_ratio(); 
//    
//    //////////////////////////////////////////////////////////////////////////
//    // Resources
//    texture_2d_ptr m_random_tex;
//    texture_2d_ptr m_ssao_buf;
//
//    //////////////////////////////////////////////////////////////////////////
//    // Effect 
//    d3dx11_effect_ptr m_hbao_effect; 
//    effect_params_table_ptr m_effect_params_table;
//    ID3DX11EffectTechnique *m_hbao_nd_tech;
//
//    ID3DX11EffectScalarVariable *m_param_radius; 
//    ID3DX11EffectScalarVariable *m_param_inv_radius; 
//    ID3DX11EffectScalarVariable *m_param_sqr_radius;
//    ID3DX11EffectScalarVariable *m_param_contrast; 
//    ID3DX11EffectScalarVariable *m_param_num_dirs; 
//    ID3DX11EffectScalarVariable *m_param_num_steps; 
//    ID3DX11EffectScalarVariable *m_param_angle_bias; 
//    ID3DX11EffectScalarVariable *m_param_tan_angle_bias; 
//    ID3DX11EffectScalarVariable *m_param_attenuation;
//    ID3DX11EffectScalarVariable *m_param_aspect_ratio;
//    ID3DX11EffectScalarVariable *m_param_inv_aspect_ratio;
//    ID3DX11EffectVectorVariable *m_param_focal_len; 
//    ID3DX11EffectVectorVariable *m_param_inv_focal_len; 
//    ID3DX11EffectVectorVariable *m_param_resolution;
//    ID3DX11EffectVectorVariable *m_param_inv_resolution;
//    
//    ID3DX11EffectShaderResourceVariable *m_linear_depth_tex_sr; 
//    ID3DX11EffectShaderResourceVariable *m_normal_tex_sr;
//    ID3DX11EffectShaderResourceVariable *m_random_tex_sr;
//
//    //////////////////////////////////////////////////////////////////////////
//    // Camera params
//    float m_fovy;
//    D3D11_VIEWPORT m_vp; 
//    
//    //////////////////////////////////////////////////////////////////////////
//    // HBAO parameters
//    int                 m_num_steps;
//    int                 m_num_dirs;
//    float               m_ao_radius;
//    float               m_radius_mutiplier;
//    float               m_angle_bias;
//    float               m_attenuation;
//    float               m_contrast;
//    float               m_aspect_ratio;
//    float               m_focal_len[2];
//    float               m_inv_focal_len[2]; 
//    float               m_resolution[2];
//    float               m_inv_resolution[2];
//};

#ifndef INV_LN2
#define INV_LN2 1.44269504f
#endif

#ifndef SQRT_LN2
#define SQRT_LN2 0.832554611f
#endif

#ifndef clamp
#define clamp(x,a,b) (min(max(x, a), b))
#endif

#define MAX_HBAO_STEP_SIZE 8
#define NUM_HBAO_PERMUTATIONS (MAX_HBAO_STEP_SIZE)

// NVSDK_HALF_RES_AO renders the AO with a half-res viewport, taking as input half-res linear depths (faster).
// NVSDK_FULL_RES_AO renders the AO with a full-res viewport, taking as input full-res linear depths (higher quality).
enum hbao_resolution_mode_enum
{
    e_half_res_ao,
    e_full_res_ao
};

struct hbao_app_params
{
    float radius;                           // default = 1.0                    // the actual eye-space AO radius is set to radius*SceneScale
    unsigned int step_size;                 // default = 4                      // step size in the AO pass // 1~16
    float angle_bias;                       // default = 10 degrees             // to hide low-tessellation artifacts // 0.0~30.0
    float stength;                          // default = 1.0                    // scale factor for the AO, the greater the darker
    float power_exponent;                   // default = 1.0                    // the final AO output is pow(AO, powerExponent)
    unsigned int blur_radius;               // default = 16 pixels              // radius of the cross bilateral filter // 0~16
    float blur_sharpness;                   // default = 8.0                    // the higher, the more the blur preserves edges // 0.0~16.0
    hbao_resolution_mode_enum m_ao_res_mode;// default = e_full_res_ao
    bool use_blending;                      // default = true                   // to multiply the AO over the destination render target
};  

class c_hbao_params_constant_buffer; 
typedef boost::shared_ptr<c_hbao_params_constant_buffer> params_cb_ptr; 
class c_hbao_input_depth_info; 
typedef boost::shared_ptr<c_hbao_input_depth_info> input_depth_info_ptr; 
class c_hbao_renderer_options;
typedef boost::shared_ptr<c_hbao_renderer_options> renderer_options_ptr;
class c_hbao_render_targets; 
typedef boost::shared_ptr<c_hbao_render_targets> render_targets_ptr;

class c_hbao_renderer_component : public c_demo_app_listener
{
    typedef c_demo_app_listener super; 

public:
    c_hbao_renderer_component(float fovy);

    virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context);
    virtual void on_release_resource();
    
    // ---------------------------------------------------------------------
    /* 
    Compile the effect
    */ 
    // ---------------------------------------------------------------------
    HRESULT compile_effects();
    HRESULT create_params_cb(); 
   
    void init_ao_params();
    void set_viewport(int width, int height);

	// ---------------------------------------------------------------------
	/*
	Update the hardware constant buffer 
	*/ 
	// ---------------------------------------------------------------------
    void set_ao_parameters(hbao_app_params& params);
    
    // ---------------------------------------------------------------------
    /* 
    Set the input depth buffer for AO rendering, called EVERY FRAME!
        - Set the constant buffer data (not updating the subresource)
        - Set the input depth buffer resolution (width, height and sample count)
        - Careful  with format of the shader resource view. 
    */ 
    // ---------------------------------------------------------------------
    void set_depth_for_ao(texture_2d_ptr& input_depth, float z_near, float z_far, float z_min, float z_max, float scene_scale);

    // ---------------------------------------------------------------------
    /*
    Linearize the input depth-stencil texture 
    Input: 
        Non-linear depth texture generated by the scene rendering. 
        The non-linear depth buffer is passed in by c_hbao_renderer_component::set_depth_for_ao() 
            which is called in onFrameRender() 
            
    Output: 
        Linear depth texture
    */
    // ---------------------------------------------------------------------
    void render_linear_depth(); 

    // ---------------------------------------------------------------------
    /*
    // Input: Linear depth buffer 
    // Output: AO Buffer
    */
    // ---------------------------------------------------------------------
    void render_ao(float fovy, d3d11_render_target_view_ptr& output_color_rtv); 
	
    
    void render_blur_x(); 
    void render_blur_y();
	
	void apply_render_composit_ps(d3d11_render_target_view_ptr& output_color_rtv); 


    //////////////////////////////////////////////////////////////////////////
    // Effect 
    d3dx11_effect_ptr m_hbao_effect; 
    effect_params_table_ptr m_hbao_effect_var_table;
    ID3DX11EffectTechnique *m_hbao_default_tech; 
    ID3DX11EffectConstantBuffer *m_hbao_effect_params_cb; 
    ID3DX11EffectShaderResourceVariable *m_input_depth_tex_sr;
	ID3DX11EffectShaderResourceVariable *m_input_random_tex_sr;
	
	d3dx11_effect_ptr m_blur_composite_effect; 
	effect_params_table_ptr m_blur_composite_effect_var_table;
	ID3DX11EffectTechnique *m_blur_composite_tech; 
	ID3DX11EffectShaderResourceVariable *m_input_ao_tex_sr;
	

    //////////////////////////////////////////////////////////////////////////
    // Resource
    
    // ---------------------------------------------------------------------
    /*
        The linear depth buffer
        Output by render_linearize_depth()
    */ 
    // ---------------------------------------------------------------------
    texture_2d_ptr m_linear_depth;
	
	// ---------------------------------------------------------------------
	/*
		Random texture 
	*/ 
	// ---------------------------------------------------------------------
	texture_2d_ptr m_random_texture;
    
    // ---------------------------------------------------------------------
    /*
        The constant buffer
    */ 
    // ---------------------------------------------------------------------
	hbao_app_params m_cached_ao_params;
    params_cb_ptr m_params_cb;
    
    // ---------------------------------------------------------------------
    /*
        HBAO renderer input textures (non-linear depth buffer rendered by scene renderer)
    */ 
    // ---------------------------------------------------------------------
    input_depth_info_ptr m_input_depth_info;
    
    // ---------------------------------------------------------------------
    /* 
        HBAO renderer settings 
    */ 
    // ---------------------------------------------------------------------
    renderer_options_ptr m_renderer_options;
    
    // ---------------------------------------------------------------------
    /*
        The output texture of render_ao()
    */ 
    // ---------------------------------------------------------------------
    render_targets_ptr m_render_targets;    

    // Viewport
    D3D11_VIEWPORT m_full_viewport;

private:
    void apply_render_ao();
	void create_random_texture();
}; 

typedef boost::shared_ptr<c_hbao_renderer_component> hbao_component_ptr; 
