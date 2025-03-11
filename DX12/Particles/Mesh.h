#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>

#include "Vertex.h"


class Mesh
{
public:
	Mesh(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount);
	Mesh(const std::wstring& objFile);

	// Getters for mesh data
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return vbView; }
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return ibView; }
	int  GetIndexCount() { return indexCount; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;
	int  indexCount;

	void CreateBuffers(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount);

	// Helper for creating buffers (in the event we add more constructor overloads)
	void CalculateTangents(Vertex* vertices, size_t vertexCount, UINT* indices, size_t indexCount);
};

