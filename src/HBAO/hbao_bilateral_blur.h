#pragma once 

#include "prerequisites.h"
#include "app_listener.h"
#include "utils.h"

class c_bilateral_blur_params_cb; 
typedef boost::shared_ptr<c_bilateral_blur_params_cb> blur_params_cb_ptr; 

struct blur_app_params
{
	blur_app_params()
		: sharpness(0)
		, blur_radius(0)
		, edge_threshold(0)
	{}
	float sharpness;
	float blur_radius; 
	float edge_threshold; 
};

class c_bilateral_blur : public c_demo_app_listener
{
	typedef c_demo_app_listener super;
public:
	
	c_bilateral_blur(); 
	~c_bilateral_blur();

	virtual HRESULT on_create_resource(const render_sys_context_ptr& render_sys_context); 
	
	virtual void on_release_resource();
	
	virtual void on_frame_render(double time, float elapsed_time); 

	virtual void on_frame_update(double time, float elapsed_time);  
	
	// void set_shader_resources()

	void set_viewport(int width, int height); 
	void set_num_samples(int num_samples); 

	void set_resolution_params(int input_width, int input_height, 
							   int output_width, int output_height);
	void set_blur_params(const blur_app_params& params); 
	void create_sr_textures(); 

	void set_resources(const texture_2d_ptr& src_tex, 
					   const texture_2d_ptr& diffuse_tex, 
					   const texture_2d_ptr& depth_tex,
					   const texture_2d_ptr& depth_msaa_tex);
	void set_dest_rtv(ID3D11RenderTargetView *dest_rtv);


	void apply_passthrough(ID3D11RenderTargetView *dest_rtv, texture_2d_ptr& diffuse_sr_tex);

private:
	
	HRESULT compile_effect();
	HRESULT create_ds_states();
	
	d3dx11_effect_ptr m_blur_effect;
	effect_params_table_ptr m_blur_effect_params;

	ID3DX11EffectTechnique *m_blur_tech; 
	ID3DX11EffectTechnique *m_blur_ss_tech;
	ID3DX11EffectTechnique *m_blur_with_diffuse_tech;
	ID3DX11EffectTechnique *m_blur_with_diffuse_ss_tech;
	ID3DX11EffectTechnique *m_edge_detect_tech;
	ID3DX11EffectTechnique *m_pass_through_tech; 
	
	ID3DX11EffectShaderResourceVariable *m_depth_tex_sr;
	ID3DX11EffectShaderResourceVariable *m_color_tex_sr;
	ID3DX11EffectShaderResourceVariable *m_src_tex_sr; 
	ID3DX11EffectShaderResourceVariable *m_depth_msaa_tex_sr;
	
	ID3DX11EffectConstantBuffer *m_effect_params_cb;
	
	D3D11_VIEWPORT m_full_viewport;

	texture_2d_ptr m_temp_buf_tex;
	texture_2d_ptr m_edge_blurred_ao; 
	texture_2d_ptr m_edge_stencil_ao; 
	
	texture_2d_ptr m_input_src_tex; 
	texture_2d_ptr m_input_depth_tex; 
	texture_2d_ptr m_input_depth_msaa_tex; 
	texture_2d_ptr m_input_diffuse_tex;
	
	
	blur_params_cb_ptr m_blur_params_cb;

	// ---------------------------------------------------------------------
	/*
		States
	*/ 
	// ---------------------------------------------------------------------
	d3d11_ds_state_ptr m_ds_default_state; 
	d3d11_ds_state_ptr m_ds_stencil_write_state;
	d3d11_ds_state_ptr m_ds_stencil_test_state;
	
	d3d11_render_target_view_ptr m_dest_rtv;

	int m_input_width, m_input_height;
	int m_output_width, m_output_height;
	int m_num_samples;
	
};

typedef boost::shared_ptr<c_bilateral_blur> hbao_bilateral_blur_ptr;