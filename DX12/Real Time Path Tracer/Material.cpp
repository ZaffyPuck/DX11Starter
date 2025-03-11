#include "Material.h"
#include "DX12Helper.h"

Material::Material(
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState,
	DirectX::XMFLOAT3 tint,
	float roughness,
	MaterialType type,
	DirectX::XMFLOAT2 uvScale,
	DirectX::XMFLOAT2 uvOffset)
	:
	pipelineState(pipelineState),
	colorTint(tint),
	roughness(roughness),
	type(type),
	uvScale(uvScale),
	uvOffset(uvOffset),
	isFinalized(false),
	highestSRVSlot(-1),
	finalGPUHandleForSRVs{}
{
	memset(textureSRVsBySlot, 0, sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * 128); // ZeroMemory
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState() { return pipelineState; }
D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForTextures() { return finalGPUHandleForSRVs; }
DirectX::XMFLOAT2 Material::GetUVScale() { return uvScale; }
DirectX::XMFLOAT2 Material::GetUVOffset() { return uvOffset; }
DirectX::XMFLOAT3 Material::GetColorTint() { return colorTint; }
float Material::GetRoughness() { return roughness; }
MaterialType Material::GetType() { return type; }

void Material::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState) { this->pipelineState = pipelineState; }
void Material::SetUVScale(DirectX::XMFLOAT2 scale) { uvScale = scale; }
void Material::SetUVOffset(DirectX::XMFLOAT2 offset) { uvOffset = offset; }
void Material::SetColorTint(DirectX::XMFLOAT3 tint) { this->colorTint = tint; }
void Material::SetRoughness(float roughness) { this->roughness = roughness; }
void Material::SetType(MaterialType type) { this->type = type; }

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle, int slot)
{
	// Valid slot?
	if (isFinalized || slot < 0 || slot >= 128)
	{
		return;
	}

	// Save and check if this was the highest slot
	textureSRVsBySlot[slot] = srvDescriptorHandle;
	highestSRVSlot = max(highestSRVSlot, slot);
}
void Material::FinalizeMaterial()
{
	if (!isFinalized && highestSRVSlot != -1)
	{
		DX12Helper& dx12Utility = DX12Helper::GetInstance();

		finalGPUHandleForSRVs = dx12Utility.CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[0], 1);;
		for (size_t i = 1; i < highestSRVSlot; i++)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dx12Utility.CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[i], 1);
		}

		isFinalized = true;
	}
}