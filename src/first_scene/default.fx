float4x4 g_mat_wvp;

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
void vs_main(   float3 pos : POSITION,
                out float4 oPosH : SV_POSITION)
{
    float4 po = float4(pos.xyz, 1); 
    oPosH = mul(po, g_mat_wvp);
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 ps_main( float4 pos : SV_POSITION ) : SV_Target
{
    return float4( 1.0f, 0.0f, 1.0f, 1.0f );    
}

technique11 render_scene
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, vs_main()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, ps_main()));
    }
}