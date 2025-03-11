#include "Sky.hlsli"

TextureCube Cube : register(t0);
SamplerState Sampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    float4 cubeSample = Cube.Sample(Sampler, input.sampleDirection);
    return cubeSample;
}