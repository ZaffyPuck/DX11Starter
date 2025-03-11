// Constant buffer
cbuffer ExternalData : register(b0)
{
	float4 colorTint;	// RGBA color
	matrix worldMatrix; // column-major
}

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{ 
	float3 localPosition	: POSITION;     // XYZ position
	float4 color			: COLOR;        // RGBA color
};

// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
	float4 color			: COLOR;        // RGBA color
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
	VertexToPixel output; // Set up output struct
	output.screenPosition = mul(worldMatrix, float4(input.localPosition, 1.0f));
	output.color = input.color * colorTint; // Pass the color through 
	return output;
}