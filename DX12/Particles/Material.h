#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

#include "Camera.h"
#include "Transform.h"

class Material
{
public:
	Material(
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
		DirectX::XMFLOAT3 tint,
		DirectX::XMFLOAT2 uvScale = DirectX::XMFLOAT2(1, 1),
		DirectX::XMFLOAT2 uvOffset = DirectX::XMFLOAT2(0, 0));

	// Getters
	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForTextures();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	DirectX::XMFLOAT3 GetColorTint();

	// Setters
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);
	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVOffset(DirectX::XMFLOAT2 offset);
	void SetColorTint(DirectX::XMFLOAT3 tint);

	// Util
	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle, int slot);
	void FinalizeMaterial();
private:
	// Material artifacts
	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT2 uvScale;

	// Pipeline state, which can be shared among materials. Includes shaders.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	// CPU
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot[128]; // is this the full 128 or 32 since there are 4 textures?
	// GPU
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs; // ref to start of array
	bool isFinalized;
	int highestSRVSlot;
};

