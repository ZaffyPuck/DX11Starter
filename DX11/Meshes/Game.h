#pragma once

#include "DXCore.h"
#include "Mesh.h"

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

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateGeometry();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Shaders and shader-related constructs
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	std::vector<std::shared_ptr<Mesh>> meshes; // holds meshes
};

