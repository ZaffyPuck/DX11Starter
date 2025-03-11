#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "Vertex.h"

class Mesh
{
private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	unsigned int indexAmount = 0;

	void CreateBuffers(Vertex* vertexArray, size_t vertexAmount, unsigned int* indexArray, size_t indexAmount, Microsoft::WRL::ComPtr<ID3D11Device> device);
	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	void Draw();
public:
	Mesh(Vertex* vertexArray, size_t vertexAmount, unsigned int* indexArray, size_t indexAmount, Microsoft::WRL::ComPtr<ID3D11Device> device);
	~Mesh();

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	unsigned int GetIndexCount();
	void SetBuffers(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	void SetBuffersAndDraw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
};

