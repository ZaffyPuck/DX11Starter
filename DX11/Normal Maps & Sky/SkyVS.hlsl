#include "Sky.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix viewMatrix; // Camera view
    matrix projectionMatrix; // Camera proj
}

VertexToPixel main(VertexShaderInput input)
{
    VertexToPixel output;
    
    matrix viewNoTranslation = viewMatrix;
    viewNoTranslation._14 = 0;
    viewNoTranslation._24 = 0;
    viewNoTranslation._34 = 0;
    
    matrix vp = mul(projectionMatrix, viewNoTranslation);
    output.screenPosition = mul(vp, float4(input.localPosition, 1.0f));
    output.screenPosition.z = output.screenPosition.w;
    
    output.sampleDirection = input.localPosition;
    return output;
}