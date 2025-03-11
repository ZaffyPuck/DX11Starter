#pragma once

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>

#include "DXCore.h"
#include "DX12Utility.h"
#include "Camera.h"
#include "Mesh.h"
#include "GameObject.h"
#include "Lights.h"

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

class Game : public DXCore
{
private:
	// Overall pipeline and rendering requirements
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	// Game entities
	int lightCount;
	std::vector<Light> lights;
	std::vector<std::shared_ptr<GameObject>> gameObjects;
	std::shared_ptr<Camera> camera;

	void LoadAssetsAndCreateEntities();
	void CreateRootSigAndPipelineState();
public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
};

