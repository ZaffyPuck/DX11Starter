#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // The local position of the vertex
	DirectX::XMFLOAT2 UV;			// Texture mapping
	DirectX::XMFLOAT3 Normal;		// Lighting
	DirectX::XMFLOAT3 Tangent;		// Normal mapping
};