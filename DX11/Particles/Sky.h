#pragma once
#include <memory> // shared_ptr
//#include <wrl/client.h> // ComPtr - in mesh
//#include <d3d11.h> // ID3D11... - in mesh
#include "Mesh.h" // Mesh
#include "SimpleShader.h" // SimplePixelShader + SimpleVertexShader
#include "Camera.h" // Camera

class Sky
{
private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;			//for sampler options
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeTextureSRV;	//for the cube map texture’s SRV
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	depthState;			//for adjusting the depth buffer comparison type
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerOptions;	//for rasterizer options(drawing the object’s “inside”)
	std::shared_ptr<Mesh> mesh;											//for the geometry to use when drawing the sky
	std::shared_ptr<SimpleVertexShader>	vertexShader;					//for the sky - specific vertex shader
	std::shared_ptr<SimplePixelShader> pixelShader;						//for the sky - specific pixel shader

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Device> device;

	// Helper for creating a cubemap from 6 individual textures. Author: Chris Cascioli
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	void Init();
public:
	Sky(
		std::shared_ptr<Mesh> mesh,
		Microsoft::WRL::ComPtr <ID3D11SamplerState> samplerOptions,
		Microsoft::WRL::ComPtr <ID3D11Device> device,
		const wchar_t* right, const wchar_t* left,
		const wchar_t* up, const wchar_t* down,
		const wchar_t* front, const wchar_t* back);
	Sky(
		std::shared_ptr<Mesh> mesh, 
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> vertexShader, 
		std::shared_ptr<SimplePixelShader> pixelShader, 
		const wchar_t* right, const wchar_t* left, 
		const wchar_t* up, const wchar_t* down, 
		const wchar_t* front, const wchar_t* back);

	void Draw(std::shared_ptr<Camera> camera);
	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
		std::shared_ptr<Camera> camera, 
		std::shared_ptr<SimpleVertexShader> vertexShader, 
		std::shared_ptr<SimplePixelShader> pixelShader);
};

