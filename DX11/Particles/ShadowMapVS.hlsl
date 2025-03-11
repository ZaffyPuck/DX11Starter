#include "HeaderStructs.hlsli"

// Constant Buffer for external (C++) data
cbuffer externalData : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

// A simplified vertex shader for rendering to a shadow map
ShadowVertexToPixel main(VertexShaderInput input)
{
    ShadowVertexToPixel output;
    matrix wvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));
    return output;
}