// Shader structures for skybox
// Basic VS input for a standard Pos/UV/Normal vertex
struct VertexShaderInput
{
    float3 localPosition : POSITION;    // looks like only this is used... can I remove others
    float3 normal : NORMAL;             // also, how does this get passed to shader? semantics?
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};
// VStoPS struct for sky box
struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float3 sampleDirection : DIRECTION; // direction in 3D space
};