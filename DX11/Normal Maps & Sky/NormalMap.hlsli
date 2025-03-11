// Shader structures for Normal maps
// Basic VS input for a standard Pos/UV/Normal vertex
struct VertexShaderInput
{
    float3 localPosition    : POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
    float3 tangent          : TANGENT;
};
// VS to PS struct for normal maps
struct VertexToPixel
{
    float4 screenPosition   : SV_POSITION;
    float3 normal           : NORMAL;
    float2 uv               : TEXCOORD;
    float3 worldPosition    : POSITION;
    float3 tangent          : TANGENT;
};