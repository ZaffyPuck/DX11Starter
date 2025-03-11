#pragma once

#include <DirectXMath.h>

#define MAX_LIGHTS 128
#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2

struct Light
{
	int	type;						// Which kind of light? 0, 1 or 2 (see above)
	DirectX::XMFLOAT3 direction;	// Directional and Spot lights need a direction
	// 16 bytes
	float range;					// Point and Spot lights have a max range for attenuation
	DirectX::XMFLOAT3 position;		// Point and Spot lights have a position in space
	// 32 bytes
	float intensity;				// All lights need an intensity
	DirectX::XMFLOAT3 color;		// All lights need a color
	// 48 bytes
	float spotFalloff;				// Spot lights need a value to define their “cone” size
	DirectX::XMFLOAT3 padding;		// Purposefully padding to hit the 16-byte boundary
	// 64 bytes
};

struct DirectLight
{
	DirectX::XMFLOAT3 direction;	// Directional and Spot lights need a direction
	float intensity;				// All lights need an intensity
	DirectX::XMFLOAT3 color;		// All lights need a color
	float padding;					// Purposefully padding to hit the 16-byte boundary
};