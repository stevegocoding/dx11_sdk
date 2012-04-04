cbuffer cb0
{
    float4x4 mat_inv_proj;
    float4 vec_depth;
}; 

Texture2D g_tex_diffuse;
Texture2D g_tex_depth;

struct post_proc_vs_output
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

SamplerState g_sampler_linear
{
    Filter = MIN_MAG_MIP_LINEAR; 
    AddressU = Clamp;
    AddressV = Clamp;
};

RasterizerState DisableCullingNoMSAA
{
    CullMode                    = NONE;
    MultiSampleEnable           = FALSE;
};

DepthStencilState DisableDepthTestWrite
{
    DepthEnable                 = FALSE;
    DepthWriteMask              = ZERO;
};

DepthStencilState DisableDepthWrite
{
    DepthEnable                 = TRUE;
    DepthWriteMask              = ZERO;
};

BlendState NoBlending
{
    AlphaToCoverageEnable       = FALSE;
    BlendEnable[0]              = FALSE;
};

post_proc_vs_output vs_dof_main(uint id : SV_VertexID)
{
    post_proc_vs_output output = (post_proc_vs_output)0.0f;
    float2 tex = float2( (id << 1) & 2, id & 2 );
    float2 screen_coord= tex * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f);
    output.position = float4( screen_coord, 0.0f, 1.0f );
    
    // Bottom left pixel is (0,0) and bottom right is (1,1)
    output.texcoord = float2( (id << 1) & 2, id & 2 );
   
    return output;
}

float4 ps_dof_main(post_proc_vs_output input, 
               uniform bool b_sample_msaa) : SV_Target
{
    int2 screen_coord = int2(input.texcoord * vec_depth.yz);
    float depth; 
    
    if (b_sample_msaa)
    {
    }
    else 
    {
        depth = g_tex_depth.Load(int3(screen_coord, 0));
    }
    
    float4 depth_sample = mul(float4(input.texcoord, depth, 1), mat_inv_proj); 
    depth = depth_sample.z / depth_sample.w;
    float blur_factor = abs(depth - vec_depth.w) / 2; 
    return g_tex_diffuse.SampleLevel(g_sampler_linear, input.texcoord, blur_factor.x); 
}

technique11 render_dof
{
    pass p0
    {
        SetVertexShader( CompileShader( vs_5_0, vs_dof_main() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, ps_dof_main( false ) ) );
        
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepthTestWrite, 0 );
        SetRasterizerState( DisableCullingNoMSAA );
    }
}