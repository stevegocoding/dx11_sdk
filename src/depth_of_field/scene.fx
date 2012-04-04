cbuffer cb0
{
    float4x4 mat_wvp;
}

Texture2D g_tex_diffuse;

SamplerState g_sampler_linear
{
    Filter = MIN_MAG_MIP_LINEAR; 
    AddressU = Clamp;
    AddressV = Clamp;
};

//////////////////////////////////////////////////////////////////////////
// Render States

DepthStencilState EnableDepthTestWrite
{
    DepthEnable                 = TRUE;
    DepthWriteMask              = ALL;
};

RasterizerState EnableCulling
{
    CullMode                    = BACK;
    MultiSampleEnable           = TRUE;
};

BlendState NoBlending
{
    AlphaToCoverageEnable       = FALSE;
    BlendEnable[0]              = FALSE;
};

struct vs_scene_input
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct vs_scene_output
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

vs_scene_output vs_scene_main(vs_scene_input input) 
{
    vs_scene_output output; 
    output.position = mul(float4(input.position, 1.0f), mat_wvp); 
    output.texcoord = input.texcoord;
    
    return output;
}

float4 ps_scene_main(vs_scene_output input) : SV_Target
{
    return g_tex_diffuse.Sample(g_sampler_linear, input.texcoord);
}

technique11 render_scene
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, vs_scene_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, ps_scene_main()));
        
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState(EnableDepthTestWrite, 0);
        SetRasterizerState(EnableCulling);
    }
}