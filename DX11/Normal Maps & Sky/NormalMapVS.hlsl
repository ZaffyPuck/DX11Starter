#include "NormalMap.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix worldMatrix;         // column-major
    matrix worldInvTpsMatrix;   // Inverse Transpose World Matrix
    matrix viewMatrix;          // Camera view
    matrix projectionMatrix;    // Camera proj
}

VertexToPixel main(VertexShaderInput input)
{
    VertexToPixel output; // Set up output struct
    matrix wvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix)); // Multiply the three matrices together first
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));
	
    output.uv = input.uv;
    output.normal = normalize(mul((float3x3) worldInvTpsMatrix, input.normal));
    output.tangent = normalize(mul((float3x3) worldMatrix, input.tangent)); // worldInvTpsMatrix // which one should I use?
    output.worldPosition = mul(worldMatrix, float4(input.localPosition, 1)).xyz;
    return output;
}