#pragma once
#include <DirectXMath.h>
#include <memory> // shared pointer
#include <unordered_map> // dictionary
#include "SimpleShader.h"
#include "Transform.h"
#include "Camera.h"

class Material
{
private:
	DirectX::XMFLOAT3 tint;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	float roughness; // 0-1

	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT2 uvScale;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
public:
	Material(
		DirectX::XMFLOAT3 tint,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader,
		float roughness
	);
	Material(
		DirectX::XMFLOAT3 tint,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader,
		float roughness,
		DirectX::XMFLOAT2 uvScale = DirectX::XMFLOAT2(1, 1),
		DirectX::XMFLOAT2 uvOffset = DirectX::XMFLOAT2(0, 0)
	);
	~Material();
	void Init(Transform* transform, std::shared_ptr<Camera> camera);

	DirectX::XMFLOAT3 GetTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	float GetRoughness();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTextureSRV(std::string name);
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSampler(std::string name);

	void SetTint(DirectX::XMFLOAT3 tint);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);
	void SetRoughness(float roughness);
	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVOffset(DirectX::XMFLOAT2 offset);

	void AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);
	void RemoveTextureSRV(std::string name);
	void RemoveSampler(std::string name);
};

