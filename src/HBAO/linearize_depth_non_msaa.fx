#include "fullscreen_triangle_vs.hlsl" 

Texture2D<float> tex_non_linear_depth; 

SamplerState point_clamp_sampler
{
    Filter = MIN_MAG_MIP_POINT; 
    AddressU = Clamp;
    AddressV = Clamp; 
}; 

DepthStencilState dss_disable_depth
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};

BlendState bs_disable_blend
{
    BlendEnable[0] = false;
};

cbuffer cb_linearize_depth_params
{
	float g_lin_a; 
	float g_lin_b; 
	float2 padding; 
};

float linearize_depth_non_msaa_ps_main(post_proc_vs_out input) : SV_TARGET
{
	float z = tex_non_linear_depth.SampleLevel(point_clamp_sampler, input.texcoord, 0); 
	return 1.0f / (z * g_lin_a + g_lin_b);
}

technique11 linearize_non_msaa_depth_tech 
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_5_0, fullscreen_triangle_vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, linearize_depth_non_msaa_ps_main()));
        
        SetBlendState(bs_disable_blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState(dss_disable_depth, 0);
	}	
}