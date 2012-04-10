#ifndef NUM_MSAA_SAMPLES
#define NUM_MSAA_SAMPLES 1
#endif

#ifndef DOWNSAMPLED
#define DOWNSAMPLED 0
#endif

#if NUM_MSAA_SAMPLES == 1
    Texture2D<float> tex_depth;
#else
    Texture2DMS<float> tex_depth;
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

struct resolve_vs_output
{
    float4 pos : SV_Position; 
    float2 tex : TEXCOORD0;
};

struct resolve_ps_output
{
    float depth : SV_Target1;
};

resolve_vs_output fullscreen_quad_vs_main(uint id : SV_VertexID)
{
    resolve_vs_output output = (resolve_vs_output)0;

    output.tex = float2( (id << 1) & 2, id & 2 );
    output.pos = float4( output.tex * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f) , 0.0f, 1.0f );
    
    return output;
}

resolve_ps_output resolve_depth_ps_main(resolve_vs_output input)
{
    resolve_ps_output output = (resolve_ps_output)0;
    
#if NUM_MSAA_SAMPLES == 1
    output.depth = tex_depth.SampleLevel(g_sampler_nearest, input.tex, 0);
#else

#if DOWNSAMPLED == 1
    output.depth = tex_depth.Load(int2(input.pos.xy)*2, 0);
#else 
    output.depth = tex_depth.Load(int2(input.pos.xy), 0);
#endif

#endif    
    return output;
}

float linearize_depth_ps(resolve_vs_output input) : SV_TARGET
{   
#if NUM_MSAA_SAMPLES == 1
    float z = tex_depth.SampleLevel(g_sampler_nearest, input.tex, 0).x;
    return z_far * z_near / (z_far - z*(z_far-z_near));
#else
    return 0; 
#endif
}

technique11 resolve_depth_tech
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, fullscreen_quad_vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, resolve_depth_ps_main()));
        
        SetBlendState(bs_disable_blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        // SetDepthStencilState(dss_disable_depth, 0);
    }
}

technique11 linearize_depth_tech
{
    pass p0
    {        
        SetVertexShader(CompileShader(vs_5_0, fullscreen_quad_vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, linearize_depth_ps()));
        
        SetBlendState(bs_disable_blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        // SetDepthStencilState(dss_disable_depth, 0); 
    }
}