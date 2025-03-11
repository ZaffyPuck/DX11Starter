#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GUI.h"

#include <DirectXMath.h>
#include <wrl/client.h> // for ComPtr
#include <vector>
#include <memory>

// Color presets
#define red XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
#define green XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
#define blue XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)
#define black XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)
#define grey XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)

// ImGUI
#include "../External/ImGUI/imgui.h"
#include "../External/ImGUI/Windows/imgui_impl_win32.h"
#include "../External/ImGUI/DX11/imgui_impl_dx11.h"
class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateGeometry();

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	std::vector<std::shared_ptr<Mesh>> meshes; // holds meshes

	void IGInit();
	void IGDelete();
	void IGUpdate(float deltaTime);
	void IGDraw();
	void IGRun();

	Microsoft::WRL::ComPtr<ID3D11Buffer> vsConstantBuffer;
	DirectX::XMFLOAT3 offset;
	DirectX::XMFLOAT4 tint;
}; 

