#pragma once

#include "DXCore.h"
#include "DX12Utility.h"
#include "Camera.h"
#include "Mesh.h"
#include "GameObject.h"

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>

class Game : public DXCore
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
	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Our scene
	std::vector<std::shared_ptr<GameObject>> gameObjects;
	std::shared_ptr<Camera> camera;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	void LoadAssetsAndCreateEntities();
	void CreateRootSigAndPipelineState();
};

