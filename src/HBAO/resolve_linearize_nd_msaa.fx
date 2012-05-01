
#include "fullscreen_triangle_vs.hlsl"

Texture2DMS<float> tex_non_linear_depth;
Texture2DMS<float4> tex_msaa_normal; 

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

struct resolve_ps_output
{
	float3 normal : SV_Target0;
    float depth : SV_Target1;
}; 

resolve_ps_output resolve_linearize_normal_depth_msaa_ps_main(post_proc_vs_out input)
{
	resolve_ps_output output = (resolve_ps_output)0;
	
	float z = tex_non_linear_depth.Load(int2(input.pos.xy), 0); 
	 
	output.depth = 1.0f / (z * g_lin_a + g_lin_b);
	output.normal = tex_msaa_normal.Load(int2(input.pos.xy), 0).xyz;
	
	return output; 
}

technique11 resolve_linearize_nd_msaa_tech
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_5_0, fullscreen_triangle_vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, resolve_linearize_normal_depth_msaa_ps_main()));
        
        SetBlendState(bs_disable_blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState(dss_disable_depth, 0);
	}	
} 