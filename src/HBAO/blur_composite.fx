
#include "hbao_params_cb.hlsl" 
#include "fullscreen_triangle_vs.hlsl"

Texture2D<float> tex_ao;
Texture2D<float> tex_diffuse;

SamplerState point_wrap_sampler
{
    Filter = MIN_MAG_MIP_POINT; 
    AddressU = Wrap;
    AddressV = Wrap;

	MipLODBias = 0.0f; 
	ComparisonFunc = NEVER; 
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

float4 blur_composite_ps_main(post_proc_vs_out input) : SV_TARGET
{
	float ao = tex_ao.Sample(point_wrap_sampler, input.texcoord); 
	return pow(ao, 1);
}

technique10 blur_composite_tech
{   
    pass p0
    {
        SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, blur_composite_ps_main() ) );
        // SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        // SetDepthStencilState( disable_depth, 0 );
    }
}