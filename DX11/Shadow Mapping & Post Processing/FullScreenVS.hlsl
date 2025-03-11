struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};
VertexToPixel main(uint id : SV_VertexID)
{
    VertexToPixel output;
    if (id == 0)
    {
        output.position = float4(-1, 1, 0, 1);
        output.uv = float2(0, 0);
    }
    else if (id == 1)
    {
        output.position = float4(3, 1, 0, 1);
        output.uv = float2(2, 0);
    }
    else if (id == 2)
    {
        output.position = float4(-1, -3, 0, 1);
        output.uv = float2(0, 2);
    }
    return output;
    
    // Branchless Fullscreen VS - uses bitwise operators for faster calculations
    //// Calculate the UV (0,0) to (2,2) using the ID
    //output.uv = float2(
    //    (id << 1) & 2, // Essentially: id % 2 * 2
    //    id & 2);
    //// Calculate the position based on the UV
    //output.position = float4(output.uv, 0, 1);
    //output.position.x = output.position.x * 2 - 1;
    //output.position.y = output.position.y * -2 + 1;
}