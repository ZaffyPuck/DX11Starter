#pragma once

#include <DirectXMath.h>

// --------------------------------------------------------
// A custom vertex definition
// --------------------------------------------------------
struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // The local position of the vertex
	DirectX::XMFLOAT2 UV;			// Texture mapping
	DirectX::XMFLOAT3 Normal;		// Lighting
	DirectX::XMFLOAT3 Tangent;		// Normal mapping
};