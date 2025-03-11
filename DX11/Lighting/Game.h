#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GUI.h"
#include "Entity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Material.h"
#include "Lights.h"

#include <DirectXMath.h>
#include <wrl/client.h> // for ComPtr
#include <vector>
#include <memory>

// Color presets
#define white4 XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
#define grey4 XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f)
#define black4 XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)
#define red4 XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
#define green4 XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)
#define blue4 XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)

#define white3 XMFLOAT3(1.0f, 1.0f, 1.0f)
#define grey3 XMFLOAT3(0.5f, 0.5f, 0.5f)
#define black3 XMFLOAT3(0.0f, 0.0f, 0.0f)
#define red3 XMFLOAT3(1.0f, 0.0f, 0.0f)
#define orange3 XMFLOAT3(1.0f, 0.5f, 0.0f)
#define yellow3 XMFLOAT3(1.0f, 1.0f, 0.0f)
#define green3 XMFLOAT3(0.0f, 1.0f, 0.0f)
#define cyan3 XMFLOAT3(0.0f, 1.0f, 1.0f)
#define blue3 XMFLOAT3(0.0f, 0.0f, 1.0f)
#define purple3 XMFLOAT3(1.0f, 0.0f, 1.0f)

// ImGUI
#include "../External/ImGUI/imgui.h"
#include "../External/ImGUI/Windows/imgui_impl_win32.h"
#include "../External/ImGUI/DX11/imgui_impl_dx11.h"

class Game : public DXCore
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
	void InitMaterials();
	void LoadObjects();
	void CreateEntities();
	void CreateLights();
	void CreateGeometry();
	void InitGraphicsState();
	void CreateConstantBuffer();
	void CreateCameras();

	// Shaders and shader-related constructs
	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimplePixelShader> customPixelShader;
	std::shared_ptr<SimplePixelShader> customPixelShader2;

	std::vector<std::shared_ptr<Mesh>> meshes;				// holds meshes
	std::vector<std::shared_ptr<Entity>> entities;			// holds entities
	std::vector<std::shared_ptr<Camera>> cameras;			// holds camera
	std::vector<std::shared_ptr<Material>> materials;		// holds materials
	std::shared_ptr<Camera> activeCamera;
	int activeCameraIndex;
	DirectX::XMFLOAT3 ambientColor = DirectX::XMFLOAT3(0.02f, 0.04f, 0.06f);
	std::vector<Light> lights;

	// ImGUI
	void IGInit();
	void IGDelete();
	void IGUpdate(float deltaTime);
	void IGDraw();
	void IGRun();

	// Helpers
	std::shared_ptr<Mesh> Create2DEquilateralTriangle(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float scale, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> Create2DRightTriangle(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float scale, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> Create2DSquare(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float size, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> CreateDefault2DSquare(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> Create2DRectangle(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float width, float height, DirectX::XMFLOAT4 color);
}; 