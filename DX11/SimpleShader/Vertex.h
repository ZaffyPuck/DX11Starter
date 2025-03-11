#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 position;	    // The local position of the vertex
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	//DirectX::XMFLOAT4 Color;        // The color of the vertex

	//Vertex(DirectX::XMFLOAT3 Position)
	//{
	//	this->position = Position;
	//	normal = DirectX::XMFLOAT3(0,0,-1);
	//	uv = DirectX::XMFLOAT2(0,0);
	//}
	//Vertex(){}
};