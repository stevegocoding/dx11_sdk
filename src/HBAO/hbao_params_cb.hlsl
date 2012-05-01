
cbuffer cb_hbao_params 
{
	float2 g_full_resolution;
	float2 g_inv_full_resolution;
	
	float2 g_ao_resolution; 
	float2 g_inv_ao_resolution;
	
	float2 g_focal_len; 
	float2 g_inv_focal_len; 
	
	float2 g_uv_to_view_a; 
	float2 g_uv_to_view_b;

	float g_r;
	float g_r2;
	float g_neg_inv_r2; 
	float g_max_radius_pixels;
	
	float g_angle_bias;
	float g_tan_angle_bias;
	float g_pow_exp;
	float g_strength;
	
	float g_blur_depth_threshold;
	float g_blur_falloff; 
	float g_lin_a; 
	float g_lin_b; 
};
