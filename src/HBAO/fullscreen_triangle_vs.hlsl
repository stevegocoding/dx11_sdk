
struct post_proc_vs_out
{
    float4 pos  : SV_Position;
    float2 texcoord    : TEXCOORD0;    
};

//----------------------------------------------------------------------------------
// Vertex shader that generates a full-screen triangle with texcoords
//----------------------------------------------------------------------------------
post_proc_vs_out fullscreen_triangle_vs_main( uint id : SV_VertexID )
{
    post_proc_vs_out output = (post_proc_vs_out)0.0f;

    // Bottom left pixel is (0,0) and bottom right is (1,1)
    output.texcoord = float2( (id << 1) & 2, id & 2 );
    
    output.pos = float4( output.texcoord * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f) , 0.0f, 1.0f );
    
    return output;
}