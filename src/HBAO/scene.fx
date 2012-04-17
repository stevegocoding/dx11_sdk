cbuffer cb0
{
    float4x4 mat_wvp;
    float4x4 mat_wv;
    float4x4 mat_wv_it;
}

cbuffer cb_constants
{
    float point_light_intensity = 1.0f;
    float kd = 1;
    float ks = 2;
    float spec_power = 200.0f;
    float4 diffuse_color = float4(0.0f, 1.0f, 1.0f, 1.0f);
    float4 spec_color = float4(1.0f, 1.0f, 1.0f, 1.0f);
}

bool g_is_ground = false;
Texture2D<float3> tex_color; 

struct vs_scene_input
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct vs_scene_output
{
    float4 hpos             : SV_POSITION;
    float3 normal           : NORMAL;
    float3 vpos             : TEXCOORD0;
    float3 key_light_pos    : TEXCOORD1;            // A global light at (0.0, 1.0, 0.0)
};

struct ps_scene_input
{
    float4 hpos             : SV_POSITION;
    float3 normal           : NORMAL;
    float3 vpos             : TEXCOORD0;
    float3 key_light_pos    : TEXCOORD1;
    bool front_facing       : SV_IsFrontFace;
};

struct ps_scene_output
{
    float4 color            : SV_Target0;
};

struct ps_scene_output_n
{
    float4 color            : SV_Target0;
    float4 normal           : SV_Target1;
};

struct ps_scene_output_nd
{
    float4 color            : SV_Target0;
    float4 normal           : SV_Target1;
    float depth             : SV_Target2;
};

struct ps_scene_output_d
{
    float4 color            : SV_Target0;
    float depth             : SV_Target1;
};

RasterizerState rs_multisample
{
    CullMode = None;
    MultisampleEnable = TRUE;
};

BlendState bs_noblending
{
    BlendEnable[0] = FALSE;
    BlendEnable[1] = FALSE;
    BlendEnable[2] = FALSE;
};

DepthStencilState ds_less_func 
{
    DepthEnable = True;
    DepthFunc = Less;
};

SamplerState point_clamp_sampler
{
    Filter = MIN_MAG_MIP_POINT; 
    AddressU = Clamp;
    AddressV = Clamp; 
}; 

vs_scene_output vs_scene_main(vs_scene_input input)
{
    vs_scene_output output;
    output.hpos = mul(float4(input.pos,1), mat_wvp);
    output.normal = normalize(mul(input.normal, (float3x3)mat_wv_it));
    output.vpos = mul(float4(input.pos,1), mat_wv).xyz;
    output.key_light_pos = mul(float4(0.0, 1.0, 0.0, 1.0), mat_wv).xyz;
    
    return output;
}

float4 calculate_shading(ps_scene_input input)
{
    if (g_is_ground)
        return float4(1,1,1,1);
    
    if (!input.front_facing)
        input.normal = -input.normal;
   
    // float4 key_light_color = float4(.457,.722 , 0.0 ,1);
    float4 key_light_color = float4(1, 1, 1, 1);
    float3 l = normalize(input.key_light_pos - input.vpos);
    float4 diffuse_key_light_color = key_light_color * (0.5 + saturate(dot(input.normal, l)) * 0.5);
    
    // fresnel term
    float3 v = -normalize(input.vpos);
    float fresnel = 0.1 + 0.9 * pow(1.0 - max(0, dot(v, input.normal)), 2.0);
    
    return (0.7 * diffuse_key_light_color + spec_color * 0.3 * fresnel);
}

ps_scene_output ps_scene_main(ps_scene_input input)
{
    ps_scene_output output = (ps_scene_output)0;
    output.color = calculate_shading(input);

    return output;
}

ps_scene_output_n ps_scene_main_n(ps_scene_input input)
{
    ps_scene_output_n output = (ps_scene_output_n)0;
    output.color = calculate_shading(input);

    // Use face normals instead of interpolated normals to make sure that the tangent planes
    // associated with the normals are actually tangent to the surface
    output.normal.xyz = normalize(cross(ddx(input.vpos.xyz), ddy(input.vpos.xyz)));
    
    return output;
}

ps_scene_output_nd ps_scene_main_nd(ps_scene_input input)
{
    ps_scene_output_nd output = (ps_scene_output_nd)0;
    output.color = calculate_shading(input);

    // Use face normals instead of interpolated normals to make sure that the tangent planes
    // associated with the normals are actually tangent to the surface
    output.normal.xyz = normalize(cross(ddx(input.vpos.xyz), ddy(input.vpos.xyz)));
    
    output.depth = input.vpos.z;
   
    return output;
}

/*
float3 brighten(float3 color)
{
	return pow(saturate(color), 0.8);
}

float4 copy_color_ps(post_proc_vs_out input) : SV_TARGET
{
	float color = tex_color.Sample(point_clamp_sampler, input.texcoord); 
	return float4(brighten(color), 1); 
}
*/

technique11 render_scene_diffuse
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, vs_scene_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, ps_scene_main_nd()));
        
		SetRasterizerState( rs_multisample );
        SetBlendState(bs_noblending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
        SetDepthStencilState(ds_less_func, 0);
    }
}

/*
technique11 copy_color
{
	pass p0
	{        
		SetVertexShader(CompileShader(vs_5_0, fullscreen_triangle_vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, copy_color_ps_main()));
        
        SetBlendState(bs_noblending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff); 
	}
}
*/