#include "ShaderStructs.hlsli"

cbuffer ExternalData : register(b0)
{
    matrix world;
    matrix worldInverseTranspose;
    matrix view;
    matrix projection;
};

VertexToPixel main(VertexShaderInput input)
{
	// Set up output struct
	VertexToPixel output;

	// Calculate output position
    matrix worldViewProj = mul(projection, mul(view, world));
    output.positionScreen = mul(worldViewProj, float4(input.positionLocal, 1.0f));

    output.uv = input.uv;
    output.normal = normalize(mul((float3x3) worldInverseTranspose, input.normal)); // ->local space
    output.tangent = normalize(mul((float3x3) world, input.tangent)); // ->world space // my other code multiplies by inverse
    output.positionWorld = mul(float4(input.positionLocal, 1.0f), world).xyz; // ->world space

	return output;
}