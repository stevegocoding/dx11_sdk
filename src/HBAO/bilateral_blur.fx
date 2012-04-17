
#ifndef NUM_MSAA_SAMPLES
#define NUM_MSAA_SAMPLES 8
#endif

#include "fullscreen_triangle_vs.hlsl"

Texture2D<float> tex_depth;					// linear dpeth 
Texture2DMS<float, NUM_MSAA_SAMPLES> tex_msaa_depth;
Texture2D<float> tex_source;				// ao texture
Texture2D<float4> tex_color;					// diffuse color texture

SamplerState point_clamp_sampler
{
    Filter   = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState linear_clamp_sampler
{
    Filter   = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

DepthStencilState disable_depth
{
    DepthEnable    = FALSE;
    DepthWriteMask = ZERO;
};

BlendState disable_blend
{
    BlendEnable[0] = false;
};

BlendState enable_blend
{
    BlendEnable[0] = true;
}; 

cbuffer cb0
{
    float2 g_resolution;
    float2 g_inv_resolution;
    float g_blur_radius;
    float g_blur_falloff;
    float g_sharpness;
    float g_edge_threshold;
};

float fetch_eye_z(float2 uv)
{
	float z = tex_depth.SampleLevel(point_clamp_sampler, uv, 0);
	return z;
} 

float blur_function(float2 uv, float r, float center_c, float center_d, inout float w_total)
{
	float c = tex_source.SampleLevel(point_clamp_sampler, uv, 0); 
	float d = fetch_eye_z(uv); 

	float ddiff = d - center_d; 
	float w = exp(-r * r * g_blur_falloff - ddiff * ddiff * g_sharpness); 
	w_total += w; 
	
	return w * c;
}

float4 blur_x(post_proc_vs_out input) : SV_TARGET
{
	 float b = 0; 
	 float w_total = 0; 
	 float center_c = tex_source.SampleLevel(point_clamp_sampler, input.texcoord, 0);
	 float center_d = fetch_eye_z(input.texcoord); 

	 for (float r = -g_blur_radius; r <= g_blur_radius; ++r)
	 {
		float2 uv = input.texcoord.xy + float2(r * g_inv_resolution.x, 0); 
		b += blur_function(uv, r, center_c, center_d, w_total);
	 }
	 
	 return b / w_total; 
}

float4 blur_y(uniform bool combine, post_proc_vs_out input) : SV_TARGET
{
	 float b = 0; 
	 float w_total = 0; 
	 float center_c = tex_source.SampleLevel(point_clamp_sampler, input.texcoord, 0);
	 float center_d = fetch_eye_z(input.texcoord); 

	 for (float r = -g_blur_radius; r <= g_blur_radius; ++r)
	 {
		float2 uv = input.texcoord.xy + float2(0, r * g_inv_resolution.y); 
		b += blur_function(uv, r, center_c, center_d, w_total); 
	 }

	 if (combine)
	 {
		return b / w_total * tex_color.SampleLevel(point_clamp_sampler, float3(input.texcoord, 0), 0);
	 }

	 return b / w_total; 
}

float4 blur_x_ss(post_proc_vs_out input) : SV_TARGET
{
	float b = 0; 
	float w_total = 0; 
	float center_c = tex_source.SampleLevel(point_clamp_sampler, input.texcoord, 0); 
	
	for (float r = -g_blur_radius; r <= g_blur_radius; ++r)
	{
		for (int s = 0; s < NUM_MSAA_SAMPLES; ++s)
		{
			float center_d = tex_msaa_depth.Load(int2(input.texcoord.xy), s); 
			float2 uv = input.texcoord.xy + float2(r * g_inv_resolution.x, 0); 
			b += blur_function(uv, r, center_c, center_d, w_total);
		}
	}
	
	return b / w_total;
}

float4 blur_y_ss(uniform bool combine, post_proc_vs_out input) : SV_TARGET
{
	float b = 0; 
	float w_total = 0; 
	float center_c = tex_source.SampleLevel(point_clamp_sampler, input.texcoord, 0);
	
	for (float r = -g_blur_radius; r <= g_blur_radius; ++r)
	{
		for (int s = 0; s < NUM_MSAA_SAMPLES; ++s)
		{
			float center_d = tex_msaa_depth.Load(int2(input.texcoord.xy), s); 
			float2 uv = input.texcoord.xy + float2(0, r * g_inv_resolution.y); 
			b += blur_function(uv, r, center_c, center_d, w_total);
		}
	}
	
	if (combine)
	{
		return b / w_total * tex_color.SampleLevel(point_clamp_sampler, float3(input.texcoord, 0), 0); 
	}

	return b / w_total;
	
}

float edge_detect_scalar(float sx, float sy, float threshold)
{
    float dist = (sx*sx+sy*sy);
    float e = (dist > threshold)? 1: 0;
    return e; 
}

float4 edge_detect_ps(uniform bool combine, post_proc_vs_out input) : SV_TARGET
{
	float3 offset = {1, -1, 0}; 
	
	float g00 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.yy * g_inv_resolution, 0).x;
    float g01 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.zy * g_inv_resolution, 0).x;
    float g02 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.xy * g_inv_resolution, 0).x;
    float g10 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.yz * g_inv_resolution, 0).x;
    float g11 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.zz * g_inv_resolution, 0).x;
    float g12 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.xz * g_inv_resolution, 0).x;
    float g20 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.yx * g_inv_resolution, 0).x;
    float g21 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.zx * g_inv_resolution, 0).x;
    float g22 = tex_depth.SampleLevel( point_clamp_sampler, input.texcoord.xy + offset.xx * g_inv_resolution, 0).x;

	// Sobel in horizontal dir
	float sx = 0;
    sx -= g00;
    sx -= g01 * 2;
    sx -= g02;
    sx += g20;
    sx += g21 * 2;
    sx += g22;

	// Sobel in vertical dir - weights are just rotated 90 degrees.
    float sy = 0;
    sy -= g00;
    sy += g02;
    sy -= g10 * 2;
    sy += g12 * 2;
    sy -= g20;
    sy += g22;

	//return g11;

    // In theory, dist should use a sqrt.  This is a common approx.
    //float greySx = 0.333 * (sx.r + sx.g + sx.b);
    //float greySy = 0.333 * (sy.r + sy.g + sy.b);
    //float eR = edgeDetectScalar(greySx, greySy);
    if( !edge_detect_scalar(sx, sy, g_edge_threshold))
	{
        discard;    
    }
    
    return 1;
} 

float4 pass_through_ps_main(post_proc_vs_out input) : SV_TARGET
{
	return tex_color.SampleLevel(point_clamp_sampler, float3(input.texcoord.xy, 0), 0);
}

technique11 blur_pass_tech
{
	pass pass_x
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_x() ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
	
	pass pass_y
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_y(false) ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}

technique11 blur_pass_with_diffuse_tech
{
	pass pass_x
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_x() ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
	
	pass pass_y
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_y(true) ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}

technique11 blur_pass_ss_tech
{
	pass pass_x
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_x_ss() ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
	
	pass pass_y
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_y_ss(false) ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}

technique11 blur_pass_with_diffuse_ss_tech
{
	pass pass_x
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_x_ss() ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
	
	pass pass_y
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_y_ss(true) ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
	}
}

technique11 blur_pass_through_tech
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, pass_through_ps_main() ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( disable_depth, 0 ); 
	}
}

technique11 blur_edge_detection
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, edge_detect_ps(false) ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( disable_depth, 0 ); 
	}
}

