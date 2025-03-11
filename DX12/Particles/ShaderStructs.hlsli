#ifndef __GGP_SHADER_STRUCTS__
#define __GPP_SHADER_STRUCTS__

// Struct representing a single vertex worth of data
struct VertexShaderInput
{
    float3 positionLocal    : POSITION; // XYZ position
    float2 uv               : TEXCOORD; // Texture mapping
    float3 normal           : NORMAL;   // Lighting
    float3 tangent          : TANGENT;  // Normal mapping
};

// Struct to pass data from shaders
struct VertexToPixel
{
    float4 positionScreen   : SV_POSITION;
    float2 uv               : TEXCOORD;     // Texture mapping
    float3 normal           : NORMAL;       // Lighting
    float3 tangent          : TANGENT;      // Normal mapping
    float3 positionWorld    : POSITION;     // World position of vertex
};

struct SkyVertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float3 sampleDirection : DIRECTION;
};
#endif