
// Struct representing the data we expect to receive from earlier pipeline stages
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
};

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is
	//   interpolated for each pixel between the corresponding vertices
	//   of the triangle we're rendering
    return float4(1, 1, 1, 1);
}