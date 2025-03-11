// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
    float4 screenPosition   : SV_POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
    float3 worldPosition    : POSITION;
};
