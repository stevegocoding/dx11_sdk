#include "hbao_params_cb.hlsl" 
#include "fullscreen_triangle_vs.hlsl"

Texture2D<float> tex_linear_depth;
Texture2D<float> tex_random;

#define M_PI 3.14159265f

#define SAMPLE_FIRST_STEP 1
#define USE_NORMAL_FREE_HBAO 0

#define RANDOM_TEXTURE_WIDTH 4
#define NUM_DIRECTIONS 8
#define NUM_STEPS 6

SamplerState point_clamp_sampler
{
    Filter = MIN_MAG_MIP_POINT; 
    AddressU = Clamp;
    AddressV = Clamp; 
}; 

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

float3 uv_to_eye(float2 uv, float eye_z)
{
	uv = g_uv_to_view_a * uv + g_uv_to_view_b;
	return float3(uv * eye_z, eye_z);  
}

// ---------------------------------------------------------------------
/*
	Get position in eye space with the uv in UV space as input 
*/ 
// ---------------------------------------------------------------------
float3 fetch_eye_pos(float2 uv)
{
    float z = tex_linear_depth.SampleLevel(point_clamp_sampler, uv, 0); 
    return uv_to_eye(uv, z); 
}

float length2(float3 v) { return dot(v, v); }
float3 min_diff(float3 p, float3 p_right, float3 p_left)
{
    float3 v1 = p_right - p;
    float3 v2 = p - p_left;
    
    return (length2(v1) < length2(v2)) ? v1 : v2; 
}

float2 rotate_direction(float2 dir, float2 cos_sin)
{
    return float2(dir.x*cos_sin.x - dir.y*cos_sin.y, 
                  dir.x*cos_sin.y + dir.y*cos_sin.x);
}

float tan_to_sin(float x)
{
    return x * rsqrt(x*x + 1.0f);
}

float tangent(float3 p, float3 s)
{
    return (p.z - s.z) / length(s.xy - p.xy);
}

float2 snap_uv_offset(float2 uv)
{
    return round(uv * g_ao_resolution) * g_inv_ao_resolution;
}

float falloff(float d2)
{
	// 1 scalar mad instruction
    return d2 * g_neg_inv_r2 + 1.0f;
}

/*
float invlength(float2 v)
{
    return rsqrt(dot(v,v));
}

float tangent(float3 t)
{
    return -t.z * invlength(t.xy);
}

float biased_tangent(float3 t)
{
    float phi = atan(tangent(t)) + g_angle_bias;
    return tan(min(phi, M_PI*0.5f)); 
}

*/

/*
float3 tangent_eye_pos(float2 uv, float4 tangent_plane)
{
    // Get the coordinates in eye space with uv
    float3 v = fetch_eye_pos(uv); 
    float3 n = float3(tangent_plane.xyz);
    float n_dot_v = dot(n, v);
    
    // Intersect with tangent plane except for silhouette edge
    if (n_dot_v < 0.0f) 
        v *= (tangent_plane.w / n_dot_v);

    return v; 
}

/*
float accumulated_hbao_lq(float2 delta_uv, float2 uv0, float3 p, float num_steps, float rand_step)
{    
    // Randomize starting point within the first sample distance
    float2 uv = uv0 + snap_uv_offset(rand_step * delta_uv); 
    
    // Snap increments to pixels to avoid disparities between xy 
    // and z sample locations and sample along a line 
    delta_uv = snap_uv_offset(delta_uv);
    
    // Add an epsilon in case g_angle_bias == 0.0f
    float tan_t = tan(-M_PI*0.5 + g_angle_bias + 1.e-5f);
    float sin_t = (g_angle_bias != 0.0) ? tan_to_sin(tan_t) : -1.0;
    
    float ao = 0.0f; 
    integrate_direction(ao, p, uv, delta_uv, num_steps, tan_t, sin_t);
    
    // Integrate opposite directions together
    delta_uv = -delta_uv;
    uv = uv0 + snap_uv_offset(rand_step * delta_uv); 
    integrate_direction(ao, p, uv, delta_uv, num_steps, tan_t, sin_t); 
    
    // Divide by 2 because we have integrated 2 directions together
    // Subtract 1 and clamp to remove the part below the surface
    return max(ao * 0.5 - 1.0, 0.0);
}
*/

/*
float accumulated_hbao(float2 delta_uv, float2 uv0, float3 p, float num_steps, float rand_step, float3 dp_du, float3 dp_dv)
{
    // Randomize starting point within the first sample distance
    float2 uv = uv0 + snap_uv_offset(rand_step * delta_uv); 
    
    // Snap increments to pixels to avoid disparities between xy 
    // and z sample locations and sample along a line   
    delta_uv = snap_uv_offset(delta_uv); 
   
    // Compute tangent vector using the tangent plane
    float3 t = delta_uv.x * dp_du + delta_uv.y * dp_dv;
    
    float tan_h = biased_tangent(t);
    float sin_h = tan_h / sqrt(1.0f + tan_h * tan_h); 
    
    float ao = 0; 
    for (float j = 1; j < num_steps; ++j)
    {
        uv += delta_uv;
        float3 s = fetch_eye_pos(uv);
        
        // Ignore any samples outside the radius of influence
        float d2 = length2(s - p);
        if (d2 < g_sqr_radius)
        {
            float tan_s = tangent(p, s);  
            [branch]
            if (tan_s > tan_h)
            {
                // Accumulate AO between the horizon and the sample
                float sin_s = tan_s / sqrt(1.0f + tan_s * tan_s);
                float r = sqrt(d2) * g_inv_radius;
                ao += falloff(r) * (sin_s - sin_h);
                
                // Update the current horizon angle
                tan_h = tan_s;
                sin_h = sin_s;
            }
        }
    }
    return ao;
}

// HBAO PS Main
float4 horizon_based_ao_ps_main(uniform bool use_normal, uniform int quality_mode, post_proc_vs_out input) : SV_TARGET
{
    float3 p =  fetch_eye_pos(input.texcoord);
    
    // Project the radius of influence g_R from eye space to texture space.
    // The scaling by 0.5 is to go from [-1,1] to [0,1].
    float2 step_size = 0.5 * g_radius * g_focal_len / p.z; 
    
    // Early out if the projected radius is smaller than 1 pixel.
    float num_steps = min(g_num_steps, min(step_size.x * g_resolution.x, step_size.y * g_resolution.y));
    if (num_steps < 1.0f) 
        return 1.0f;
    step_size = step_size / (num_steps + 1);
    
    // Nearest neighbor pixels on the tangent plane
    float3 p_left, p_right, p_top, p_bottom;
    float4 tangent_plane;
    if (use_normal)
    {
        float3 n = normalize(tex_normal.SampleLevel(point_clamp_sampler, float3(input.texcoord,0), 0));
        // plane equation four coeffs
        tangent_plane = float4(n, dot(p, n)); 
        p_left = tangent_eye_pos(input.texcoord + float2(-g_inv_resolution.x, 0), tangent_plane);
        p_right = tangent_eye_pos(input.texcoord + float2(g_inv_resolution.x, 0), tangent_plane);
        p_top = tangent_eye_pos(input.texcoord + float2(0, g_inv_resolution.y), tangent_plane);
        p_bottom = tangent_eye_pos(input.texcoord + float2(0, -g_inv_resolution.y), tangent_plane);
    }
    else 
    {
        p_left = fetch_eye_pos(input.texcoord + float2(-g_inv_resolution.x, 0));
        p_right = fetch_eye_pos(input.texcoord + float2(g_inv_resolution.x, 0));
        p_top = fetch_eye_pos(input.texcoord + float2(0, g_inv_resolution.y));
        p_bottom = fetch_eye_pos(input.texcoord + float2(0, -g_inv_resolution.y));
        float3 n = normalize(cross(p_right - p_left, p_top - p_bottom)); 
        tangent_plane = float4(n, dot(p, n));
    }
   
    // Screen-aligned basis for the tangent plane
    float3 dp_du = min_diff(p, p_right, p_left);
    float3 dp_dv = min_diff(p, p_top, p_bottom) * (g_resolution.y * g_inv_resolution.x);
    
    // (cos(alpha),sin(alpha),jitter)
    float3 rand = tex_random.Load(int3((int)input.pos.x&63, (int)input.pos.y&63, 0));
    
    float ao = 0;
    float d;
    float alpha = 2.0f * M_PI / g_num_dirs;
    
    switch (quality_mode)
    {
    case 0:
        for (d = 0; d < g_num_dirs * 0.5f; ++d)
        {
            float angle = alpha * d;
            float2 dir = float2(cos(angle), sin(angle)); 
            float2 delta_uv = rotate_direction(dir, rand.xy) * step_size.xy;
            // ao += accumulated_hbao_lq(delta_uv, input.texcoord, p, num_steps, rand.z);
        }
        ao *= 2.0; 
        break;
    
    case 1:
        for (d = 0; d < g_num_dirs; ++d) 
        {
            float angle = alpha * d; 
            float2 dir = float2(cos(angle), sin(angle));
            float2 delta_uv = rotate_direction(dir, rand.xy) * step_size.xy; 
        }
        break;
    }
    
    return 1.0 - ao / g_num_dirs * g_contrast;
}
*/ 

void integrate_direction(inout float ao, float3 p, float2 uv, float2 delta_uv, float num_steps, float tan_h, float sin_h)
{
	for (float j = 1; j <= num_steps; ++j)
	{
		uv += delta_uv;
		float3 s = fetch_eye_pos(uv); 
		float tan_s = tangent(p, s); 
		float d2 = length2(s - p); 
		
		[branch]
		if ( (d2 < g_r2) && (tan_s > tan_h) )
		{
			// Accumulate AO between horizon and the sample 
			float sin_s = tan_to_sin(tan_s); 
			ao += falloff(d2) * (sin_s - sin_h); 
			
			// Update the current horizon angle
			tan_h = tan_s; 
			sin_h = sin_s; 
		}
	}
}

void compute_steps(inout float2 step_size_uv, inout float num_steps, float ray_radius_pix, float rand)
{
    // Avoid oversampling if NUM_STEPS is greater than the kernel radius in pixels
	num_steps = min(NUM_STEPS, ray_radius_pix);
	
	float step_size_pix = ray_radius_pix / (num_steps + 1);

	float max_num_steps = g_max_radius_pixels / step_size_pix; 
	
    // Clamp num_steps if it is greater than the max kernel footprint	
	if (max_num_steps < num_steps)
	{
		// Use dithering to avoid AO discontinuities
		num_steps = floor(max_num_steps + rand);
		num_steps = max(num_steps, 1); 
		step_size_pix = g_max_radius_pixels / num_steps; 
	}
	
	// 
	step_size_uv = step_size_pix * g_inv_ao_resolution; 
}

float normal_free_horizon_occlusion(float2 delta_uv, 
									float2 texel_delta_uv, 
									float2 uv0,
									float3 p, 
									float num_steps,
									float rand_step)
{
	float tan_t = tan(-M_PI * 0.5 + g_angle_bias);
	float sin_t = tan_to_sin(tan_t); 
	 
#if SAMPLE_FIRST_STEP 
	float ao1 = 0; 
	
	// Take the first sample between uv0 and uv0 + delta_uv
	float2 delta_uv1 = snap_uv_offset(rand_step * delta_uv + texel_delta_uv); 

	integrate_direction(ao1, p, uv0, delta_uv1, 1, tan_t, sin_t); 
	integrate_direction(ao1, p, uv0, -delta_uv1, 1, tan_t, sin_t);
	
	ao1 = max(ao1 * 0.5 - 1.0, 0.0); 
	--num_steps;
	
#endif
	
	float ao = 0; 
	float2 uv = uv0 + snap_uv_offset(rand_step * delta_uv); 
	delta_uv = snap_uv_offset(delta_uv); 
	integrate_direction(ao, p, uv, delta_uv, num_steps, tan_t, sin_t); 

	// Integrate opposite directions together
	delta_uv = -delta_uv; 
	uv = uv0 + snap_uv_offset(rand_step * delta_uv);
	integrate_direction(ao, p, uv, delta_uv, num_steps, tan_t, sin_t);

    // Divide by 2 because we have integrated 2 directions together
    // Subtract 1 and clamp to remove the part below the surface
#if SAMPLE_FIRST_STEP
    return max(ao * 0.5 - 1.0, ao1);
#else
    return max(ao * 0.5 - 1.0, 0.0);
#endif

}

float2 test_ps(post_proc_vs_out input) : SV_TARGET
{
	float3 p = fetch_eye_pos(input.texcoord); 

	// float3 rand = tex_random.SampleLevel(point_wrap_sampler, input.pos.xy / RANDOM_TEXTURE_WIDTH, 0);

	float3 rand = (float3)0.3; 

	float3 temp = rand * (float)1;
	
	// Compute projection of disk of radius g_R into uv space
    // Multiply by 0.5 to scale from [-1,1]^2 to [0,1]^2
	float2 ray_radius_uv = 0.5 * g_r * g_focal_len / p.z; 
	float ray_radius_pix = ray_radius_uv.x * g_ao_resolution.x;
	if (ray_radius_pix < 1)
		return 1.0; 

	// Determine the sampling steps 
	float num_steps;
	float2 step_size = (float2)0;			// step size in U-V space
	compute_steps(step_size, num_steps, ray_radius_pix, rand.z);

    // Nearest neighbor pixels on the tangent plane
    float3 p_left, p_right, p_top, p_bottom;
    float4 tangent_plane;
	p_right = fetch_eye_pos(input.texcoord + float2(g_inv_ao_resolution.x, 0)); 
	p_left = fetch_eye_pos(input.texcoord + float2(-g_inv_ao_resolution.x, 0)); 
	p_top = fetch_eye_pos(input.texcoord + float2(0, g_inv_ao_resolution.y)); 
	p_bottom = fetch_eye_pos(input.texcoord + float2(0, -g_inv_ao_resolution.y)); 
	
	// Screen-aligned basis for the tangent plane
    float3 dPdu = min_diff(p, p_right, p_left);
    float3 dPdv = min_diff(p, p_top, p_bottom) * (g_ao_resolution.y * g_inv_ao_resolution.x);
	
	// Calculate the AO for each direction 
	float ao = 0; 
	float d; 
	float alpha = 2.0f * M_PI / NUM_DIRECTIONS; 
    for (d = 0; d < NUM_DIRECTIONS*0.5; ++d)
    {
		float angle = alpha * d; 
		float2 dir = rotate_direction(float2(cos(angle), sin(angle)), rand.xy); 
		float2 delta_uv = dir * step_size.xy;
		float2 texel_delta_uv = dir * g_inv_ao_resolution; 
		ao += normal_free_horizon_occlusion(delta_uv, texel_delta_uv, input.texcoord, p, num_steps, rand.z); 
	}
	ao *= 2.0;

	ao = 1.0 - ao / NUM_DIRECTIONS * g_strength; 

	return float2(ao, p.z); 
}


technique10 hbao_default_tech
{   
    pass p0
    {
        SetVertexShader( CompileShader( vs_5_0, fullscreen_triangle_vs_main() ) );
        SetGeometryShader( NULL );
        //SetPixelShader( CompileShader( ps_5_0, horizon_based_ao_ps_main(false, 1) ) );
		SetPixelShader( CompileShader( ps_5_0, test_ps() ) );
        SetBlendState( disable_blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( disable_depth, 0 );
    }
}