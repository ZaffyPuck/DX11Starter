#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>

#include "Vertex.h"

struct MeshRaytracingData
{
	D3D12_GPU_DESCRIPTOR_HANDLE IndexbufferSRV{ };
	D3D12_GPU_DESCRIPTOR_HANDLE VertexBufferSRV{ };
	Microsoft::WRL::ComPtr<ID3D12Resource> BLAS;
	unsigned int HitGroupIndex = 0;
};

class Mesh
{
public:
	Mesh(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount);
	Mesh(const std::wstring& objFile);

	// Getters for mesh data
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return vbView; }
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return ibView; }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexBufferResource() { return vertexBuffer; }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexBufferResource() { return indexBuffer; }
	int  GetIndexCount() { return indexCount; }
	int  GetVertexCount() { return vertexCount; }

	MeshRaytracingData GetRaytracingData() { return raytracingData; }

private:
	int indexCount;
	int vertexCount;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;

	D3D12_INDEX_BUFFER_VIEW ibView;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	MeshRaytracingData raytracingData;

	void CreateBuffers(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount);
	// Helper for creating buffers (in the event we add more constructor overloads)
	void CalculateTangents(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount);
};

