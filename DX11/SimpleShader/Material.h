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
public:
	Material(
		DirectX::XMFLOAT3 tint,
		std::shared_ptr<SimpleVertexShader> vertexShader,
		std::shared_ptr<SimplePixelShader> pixelShader
	);
	~Material();

	DirectX::XMFLOAT3 GetTint();
	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	std::shared_ptr<SimplePixelShader> GetPixelShader();
	void SetTint(DirectX::XMFLOAT3 tint);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertexShader);
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixelShader);
};

