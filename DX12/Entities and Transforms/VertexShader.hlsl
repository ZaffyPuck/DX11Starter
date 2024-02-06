cbuffer externalData : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
};

// Struct representing a single vertex worth of data
struct VertexShaderInput
{
    float3 position : POSITION; // XYZ position
    float2 uv       : TEXCOORD;	// Texture mapping
	float3 normal	: NORMAL;	// Lighting
    float3 tangent  : TANGENT;	// Normal mapping
};

// Struct representing the data we're sending down the pipeline
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
};

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// --------------------------------------------------------
VertexToPixel main( VertexShaderInput input )
{
	// Set up output struct
	VertexToPixel output;

	// Calculate output position
    matrix worldViewProj = mul(projection, mul(view, world));
    output.screenPosition = mul(worldViewProj, float4(input.position, 1.0f));

	return output;
}