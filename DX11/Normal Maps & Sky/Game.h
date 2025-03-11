#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GUI.h"
#include "Entity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Material.h"
#include "Lights.h"
#include "Sky.h"

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
// position presets
//#define center XMFLOAT3()

// ImGUI
#include "../External/ImGUI/imgui.h"
#include "../External/ImGUI/Windows/imgui_impl_win32.h"
#include "../External/ImGUI/DX11/imgui_impl_dx11.h"
#include "WICTextureLoader.h" // DXTK needed for this

class Game : public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
private:
	// Fields
	std::vector<std::shared_ptr<SimpleVertexShader>> vertexShaders; // Vertex Shaders
	std::vector<std::shared_ptr<SimplePixelShader>> pixelShaders;	// Pixel Shaders
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;				// Sampler State
	std::vector<std::shared_ptr<Mesh>> meshes;						// Meshes
	std::vector<std::shared_ptr<Material>> materials;				// Materials
	std::vector<std::shared_ptr<Entity>> entities;					// Entities
	std::vector<std::shared_ptr<Camera>> cameras;					// Cameras
	std::vector<Light> lights;										// Lights
	std::shared_ptr<Camera> activeCamera;							// Active Cemera
	int activeCameraIndex{};										// Active Camera Index
	DirectX::XMFLOAT3 ambientColor = DirectX::XMFLOAT3(0.1f, 0.1f, 0.2f); // Ambient Color
	std::shared_ptr<Sky> sky;

	// Helper functions
	void LoadShaders();
	void InitMaterials();
	void LoadTextures();
	void LoadObjects();
	void InitSkyBox();
	void CreateEntities();
	void CreateLights();
	void CreateGeometry();
	void InitGraphicsState();
	void CreateConstantBuffer();
	void CreateCameras();

	// ImGUI
	void IGInit();
	void IGDelete();
	void IGUpdate(float deltaTime);
	void IGDraw();
	void IGRun();

	// Geometry
	std::shared_ptr<Mesh> Create2DEquilateralTriangle(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float scale, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> Create2DRightTriangle(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float scale, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> Create2DSquare(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float size, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> CreateDefault2DSquare(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT4 color);
	std::shared_ptr<Mesh> Create2DRectangle(Microsoft::WRL::ComPtr<ID3D11Device> device, DirectX::XMFLOAT3 position, float width, float height, DirectX::XMFLOAT4 color);
}; 