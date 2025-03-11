#ifndef __GGP_SHADER_STRUCTS__
#define __GPP_SHADER_STRUCTS__

struct VertexShaderInput
{
    float3 localPosition    : POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
    float3 tangent          : TANGENT;
};

struct VertexToPixel
{
    float4 screenPosition   : SV_POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
    float3 worldPosition    : POSITION;
    float3 tangent          : TANGENT;
    float4 shadowMapPos     : SHADOW_POSITION;
};

struct SkyVertexToPixel
{
    float4 screenPosition   : SV_POSITION;
    float3 sampleDirection  : DIRECTION;
};

struct ShadowVertexToPixel
{
    float4 screenPosition   : SV_POSITION;
};
#endif