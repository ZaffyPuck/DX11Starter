#include "Mesh.h"
#include <DirectXMath.h>
#include <vector>
#include <fstream>

//using namespace DirectX;

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer()
{
	return vertexBuffer;
}
Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer()
{
	return indexBuffer;
}
unsigned int Mesh::GetIndexCount()
{
	return indexAmount;
}

void Mesh::SetBuffers(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) {
	// Set buffers in the input assembler
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

Mesh::Mesh(Vertex* vertexArray, size_t vertexAmount, unsigned int* indexArray, size_t indexAmount, Microsoft::WRL::ComPtr<ID3D11Device> device)
{
	CreateBuffers(vertexArray, vertexAmount, indexArray, indexAmount, device);
}
Mesh::~Mesh()
{
}

void Mesh::CreateBuffers(Vertex* vertexArray, size_t vertexAmount, unsigned int* indexArray, size_t indexAmount, Microsoft::WRL::ComPtr<ID3D11Device> device)
{
	// -- Create the vertex buffer -- //
	D3D11_BUFFER_DESC vbd = {};
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * (UINT)vertexAmount; // Number of vertices
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA initialVertexData = {};
	initialVertexData.pSysMem = vertexArray;
	
	device->CreateBuffer(&vbd, &initialVertexData, vertexBuffer.GetAddressOf());

	// -- Create the index buffer -- //
	D3D11_BUFFER_DESC ibd = {};
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * (UINT)indexAmount; // Number of indices
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA initialIndexData = {};
	initialIndexData.pSysMem = indexArray;

	device->CreateBuffer(&ibd, &initialIndexData, indexBuffer.GetAddressOf());

	// Save the indices
	this->indexAmount = (unsigned int)indexAmount;
}
void Mesh::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) {
	context->DrawIndexed(this->indexAmount, 0, 0);
}
void Mesh::SetBuffersAndDraw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	SetBuffers(context);
	Draw(context);
}