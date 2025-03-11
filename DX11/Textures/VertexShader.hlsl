#include "HeaderStructs.hlsli"

// Constant buffer
cbuffer ExternalData : register(b0)
{
	matrix worldMatrix;			// column-major
    matrix worldInvTpsMatrix;	// Inverse Transpose World Matrix
	matrix viewMatrix;			// Camera view
	matrix projectionMatrix;	// Camera proj
}

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{ 
	float3 localPosition	: POSITION;
    float3 normal			: NORMAL;
    float2 uv				: TEXCOORD;
};

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	VertexToPixel output;		// Set up output struct
	matrix wvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix)); // Multiply the three matrices together first
	output.screenPosition = mul(wvp, float4(input.localPosition, 1.0f));
	
    output.normal = normalize(mul((float3x3) worldInvTpsMatrix, input.normal)); // Perfect!
    output.uv = input.uv;
    output.worldPosition = mul(worldMatrix, float4(input.localPosition, 1)).xyz;
	return output;
}