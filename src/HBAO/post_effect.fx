
SamplerState g_sampler_linear
{
    Filter = MIN_MAG_MIP_LINEAR;
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

struct std_quad_vs_output
{
    float4 pos : SV_Position; 
    float2 tex : TEXCOORD0;
};

struct std_quad_ps_output
{
    float3 normal : SV_Target0;
};

Texture2D<float3> tex_source;

std_quad_vs_output std_quad_vs_main(uint id : SV_VertexID)
{
    std_quad_vs_output output = (std_quad_vs_output)0;

    output.tex = float2( (id << 1) & 2, id & 2 );
    output.pos = float4( output.tex * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f) , 0.0f, 1.0f );
    
    return output;
}

float4 std_quad_ps_main(std_quad_vs_output input) : SV_Target0
{
    float3 color;
    color = tex_source.SampleLevel(g_sampler_linear, input.tex, 0);
    return float4(color.rgb, 1.0f);
}

technique std_quad_tech
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, std_quad_vs_main())); 
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, std_quad_ps_main())); 
        
        SetBlendState(bs_disable_blend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState(dss_disable_depth, 0); 
    }
}