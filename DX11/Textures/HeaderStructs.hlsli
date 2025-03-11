#ifndef __GGP_SHADER_INCLUDES__
#define __GGP_SHADER_INCLUDES__
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2
#define MAX_SPECULAR_EXPONENT 256.0f

struct Light
{
    int type;
    float3 direction;
    float range;
    float3 position;
    float intensity;
    float3 color;
    float spotFalloff;
    float3 padding;
};

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


#endif