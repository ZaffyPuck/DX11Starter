#pragma once
#include <DirectXMath.h>
#include <memory> // shared pointer
#include "SimpleShader.h"

class Material
{
private:
	DirectX::XMFLOAT3 tint;
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	float roughness; // 0-1
public:
	Material(
		DirectX::XMFLOAT3 tint,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader,
		float roughness
	);
	~Material();

	DirectX::XMFLOAT3 GetTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	float GetRoughness();

	void SetTint(DirectX::XMFLOAT3 tint);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);
	void SetRoughness(float roughness);
};

