#ifndef NUM_MSAA_SAMPLES 
#define NUM_MSAA_SAMPLES 1
#endif 

#ifndef DOWNSAMPLED
#define DOWNSAMPLED 0
#endif

#include "fullscreen_triangle_vs.hlsl" 

#if NUM_MSAA_SAMPLES == 1
	Texture2D<float> tex_depth;
	Texture2D<float3> tex_normal; 
#else 
	Texture2DMS<float3> tex_normal;
	Texture2DMS<float>  tex_depth;
#endif 

cbuffer cb_proj_plane
{
    float z_near;
    float z_far;
}

SamplerState g_sampler_nearest
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

struct resolve_ps_output
{
	float3 normal : SV_Target0;
    float depth : SV_Target1;
}; 

resolve_ps_output resolve_nd_ps_main(post_proc_vs_out input)
{
    resolve_ps_output output = (resolve_ps_output)0;
    
#if NUM_MSAA_SAMPLES == 1
    output.depth = tex_depth.SampleLevel(g_sampler_nearest, input.texcoord, 0);
	output.normal = tex_normal.SampleLevel(g_sampler_nearest, input.texcoord, 0);
#else

#if DOWNSAMPLED == 1
    output.depth = tex_depth.Load(int2(input.pos.xy)*2, 0);
	output.normal = tex_normal.Load(int2(input.pos.xy)*2, 0);
#else 
    output.depth = tex_depth.Load(int2(input.pos.xy), 0);
	output.normal = tex_normal.Load(int2(input.pos.xy), 0);
#endif

#endif    
    return output; 	
}

resolve_ps_output resolve_linearize_nd_main(post_proc_vs_out input) : SV_TARGET
{
    resolve_ps_output output = (resolve_ps_output)0;
    
#if NUM_MSAA_SAMPLES == 1
    float z = tex_depth.SampleLevel(g_sampler_nearest, input.texcoord, 0);
	output.normal = tex_normal.SampleLevel(g_sampler_nearest, input.texcoord, 0);
#else

#if DOWNSAMPLED == 1
    float z = tex_depth.Load(int2(input.pos.xy)*2, 0);
	output.normal = tex_normal.Load(int2(input.pos.xy)*2, 0);
#else 
    float z = tex_depth.Load(int2(input.pos.xy), 0);
	output.normal = tex_normal.Load(int2(input.pos.xy), 0);
#endif

#endif    
	output.depth = z_far * z_near / (z_far - z*(z_far-z_near)); 
	
    return output; 
} 

technique11 resolve_nd_tech
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_5_0, fullscreen_triangle_vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, resolve_nd_ps_main()));
        
        SetBlendState(bs_disable_blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState(dss_disable_depth, 0);
	}
}

technique11 resolve_linearize_nd_tech
{
	pass p0
	{
		SetVertexShader(CompileShader(vs_5_0, fullscreen_triangle_vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, resolve_linearize_nd_main()));
        
        SetBlendState(bs_disable_blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState(dss_disable_depth, 0);
	}
	
}