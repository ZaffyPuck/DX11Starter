#include <stdlib.h>     // For seeding random and rand()
#include <time.h>       // For grabbing time (to seed random)
#include <d3dcompiler.h>

#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "BufferStructs.h"
#include "RayTracingHelper.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")

// For the DirectX Math library
using namespace DirectX;

XMFLOAT3 GetRandomFloat3()
{
	float r = (float)rand() / RAND_MAX;
	float g = (float)rand() / RAND_MAX;
	float b = (float)rand() / RAND_MAX;
	return XMFLOAT3(r,g,b);
}

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true),				// Show extra stats (fps) in title bar?
	lightCount(8),
	raysPerPixel(25),
	maxRecursionDepth(10)
{
	// Seed random
	srand((unsigned int)time(0));

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// We need to wait here until the GPU
	// is actually done with its work
	DX12Helper::GetInstance().WaitForGPU();
	delete& RaytracingHelper::GetInstance();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Attempt to initialize DXR
	RaytracingHelper::GetInstance().Initialize(
		windowWidth,
		windowHeight,
		device,
		commandQueue,
		commandList,
		FixPath(L"Raytracing.cso"));

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	// - You'll be expanding and/or replacing these later
	CreateRootSigAndPipelineState();

	// Asset loading and entity creation
	LoadAssetsAndCreateGameObjects();

	GenerateLights();

	// Create the camera
	float aspectRatio = (float)windowWidth / windowHeight;
	camera = std::make_shared<Camera>(
		0.0f, 3.0f, -15.0f,	// Position
		5.0f,				// Move speed (world units)
		0.002f,				// Look speed (cursor movement pixels --> radians for rotation)
		aspectRatio,		// Aspect Ratio
		XM_PIDIV4);			// Field of view

	commandList->Close();
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), pixelShaderByteCode.GetAddressOf());
	}
	// Input layout
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[0].SemanticName = "POSITION"; // Name must match semantic in shader
		inputElements[0].SemanticIndex = 0; // This is the first POSITION semantic

		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT; // R32 G32 = float2
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].SemanticIndex = 0; // This is the first TEXCOORD semantic

		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].SemanticIndex = 0; // This is the first NORMAL semantic

		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT; // R32 G32 B32 = float3
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].SemanticIndex = 0; // This is the first TANGENT semantic
	}
	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4; // Set to max number of textures at once (match pixel shader!)
		srvRange.BaseShaderRegister = 0; // Starts at s0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};

		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;

		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;

		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;

		// Create a single static sampler (available to all pixel shaders at the same slot)
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0; // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap };

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);

		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((wchar_t*)errors->GetBufferPointer());
		}

		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}
	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();

		// -- Shaders (VS/PS) ---
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}

// --------------------------------------------------------
// Load all assets and create materials, entities, etc.
// --------------------------------------------------------
void Game::LoadAssetsAndCreateGameObjects()
{
	// Get meshes
	std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str());
	std::shared_ptr<Mesh> helixMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str());
	std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str());
	std::shared_ptr<Mesh> coneMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cone.obj").c_str());
	std::shared_ptr<Mesh> torusMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str());

	// Create static game objects
	std::shared_ptr<Material> groundMaterial = std::make_shared<Material>(pipelineState, grey3, 1.f, MaterialType::Default);
	std::shared_ptr<GameObject> ground = std::make_shared<GameObject>(cubeMesh, groundMaterial);
	ground->GetTransform()->SetScale(50.f,0.2f,50.f);
	ground->GetTransform()->SetPosition(0, -0.1, 0);

	std::shared_ptr<Material> torusMaterial = std::make_shared<Material>(pipelineState, white3, 0.f);
	std::shared_ptr<GameObject> bigTorus = std::make_shared<GameObject>(torusMesh, torusMaterial);
	bigTorus->GetTransform()->SetScale(3);
	bigTorus->GetTransform()->SetRotation(XM_PIDIV2, 0.f, 0.f); // 90 deg
	bigTorus->GetTransform()->SetPosition(0, 6, 0);

	gameObjects.push_back(ground);
	gameObjects.push_back(bigTorus);

	// Create dynamic game objects
	float dynmaicObjectHeight = 3.0f;
	std::shared_ptr<Material> sphere1Material = std::make_shared<Material>(pipelineState, GetRandomFloat3(), 0.f);
	std::shared_ptr<GameObject> sphere1 = std::make_shared<GameObject>(sphereMesh, sphere1Material);
	sphere1->GetTransform()->SetPosition(-6, dynmaicObjectHeight, 0);
	sphere1->GetTransform()->SetScale(2);

	std::shared_ptr<Material> helix1Material = std::make_shared<Material>(pipelineState, GetRandomFloat3(), 0.33f);
	std::shared_ptr<GameObject> helix1 = std::make_shared<GameObject>(helixMesh, helix1Material);
	helix1->GetTransform()->SetPosition(-2, dynmaicObjectHeight, 0);

	std::shared_ptr<Material> cube1Material = std::make_shared<Material>(pipelineState, GetRandomFloat3(), 0.66f);
	std::shared_ptr<GameObject> cube1 = std::make_shared<GameObject>(cubeMesh, cube1Material);
	cube1->GetTransform()->SetPosition(2, dynmaicObjectHeight, 0);

	std::shared_ptr<Material> cone1Material = std::make_shared<Material>(pipelineState, GetRandomFloat3(), 1.f);
	std::shared_ptr<GameObject> cone1 = std::make_shared<GameObject>(coneMesh, cone1Material);
	cone1->GetTransform()->SetPosition(6, dynmaicObjectHeight, 0);

	gameObjects.push_back(sphere1);
	gameObjects.push_back(helix1);
	gameObjects.push_back(cube1);
	gameObjects.push_back(cone1);
	// add to list to get transformed
	dynamicGameObjects.push_back(sphere1);
	dynamicGameObjects.push_back(helix1);
	dynamicGameObjects.push_back(cube1);
	dynamicGameObjects.push_back(cone1);

	// Create Spheres
	float minPos = -20.f;
	float maxPos = 20.f;
	float minScale = 0.8f;
	float maxScale = 2.5f;
	for (size_t i = 0; i < 20; i++)
	{
		float randRoughness = (float)rand() / RAND_MAX; // 0-1
		std::shared_ptr<Material> sphereMaterial = std::make_shared<Material>(pipelineState, GetRandomFloat3(), randRoughness);
		std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>(sphereMesh, sphereMaterial);
		// Scale
		float scale = (float)rand() / RAND_MAX * (maxScale - minScale) + minScale;
		sphere->GetTransform()->SetScale(scale);
		// Position
		float xPos = (float)rand() / RAND_MAX * (maxPos - minPos) + minPos;
		float yPos = scale / 2.f;
		float zPos = (float)rand() / RAND_MAX * (maxPos - minPos) + minPos;
		sphere->GetTransform()->SetPosition(xPos, yPos, zPos);
		// Add
		gameObjects.push_back(sphere);
	}

	for (size_t i = 0; i < 5; i++)
	{
		float randIntensity = ((float)rand() / RAND_MAX) + 1; // 1-2
		std::shared_ptr<Material> sphereEmissiveMaterial = std::make_shared<Material>(pipelineState, GetRandomFloat3(), randIntensity, MaterialType::Emissive);
		std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>(sphereMesh, sphereEmissiveMaterial);
		// Scale
		sphere->GetTransform()->SetScale(0.6f);
		// Position
		float xPos = (float)rand() / RAND_MAX * (maxPos - minPos) + minPos;
		float yPos = 0.3f;
		float zPos = (float)rand() / RAND_MAX * (maxPos - minPos) + minPos;
		sphere->GetTransform()->SetPosition(xPos, yPos, zPos);
		// Add
		gameObjects.push_back(sphere);
	}

	for (size_t i = 0; i < 5; i++)
	{
		std::shared_ptr<Material> sphereTransparentMaterial = std::make_shared<Material>(pipelineState, GetRandomFloat3(), 0.0f, MaterialType::Transparent);
		std::shared_ptr<GameObject> sphere = std::make_shared<GameObject>(sphereMesh, sphereTransparentMaterial);
		// Scale
		sphere->GetTransform()->SetScale(1.2f);
		// Position
		float xPos = (float)rand() / RAND_MAX * (maxPos - minPos) + minPos;
		float yPos = 0.6f;
		float zPos = (float)rand() / RAND_MAX * (maxPos - minPos) + minPos;
		sphere->GetTransform()->SetPosition(xPos, yPos, zPos);
		// Add
		gameObjects.push_back(sphere);
	}

	// Create the TLAS for the scene. Meshes create their own BLAS's
	RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(gameObjects);
}

void Game::GenerateLights()
{
	lights.clear();

	Light dLight1 = {};
	dLight1.type = LIGHT_TYPE_DIRECTIONAL;
	dLight1.direction = DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f);
	dLight1.color = white3;
	dLight1.intensity = 1.0f;

	lights.push_back(dLight1);

	lights.resize(MAX_LIGHTS);

	directLight = {};
	directLight.direction = DirectX::XMFLOAT3(0.3f, -1.0f, -0.5f);
	directLight.intensity = 1.0f;
	directLight.color = white3;
}

// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update our projection matrix to match the new aspect ratio
	if (camera)
	{
		float aspectRatio = (float)windowWidth / windowHeight;
		camera->UpdateProjectionMatrix(aspectRatio);
	}

	RaytracingHelper::GetInstance().ResizeOutputUAV(windowWidth, windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Check individual input
	Input& input = Input::GetInstance();
	if (input.KeyDown(VK_ESCAPE)) Quit();

	float y = sinf(totalTime);

	for (size_t i = 0; i < dynamicGameObjects.size(); i++)
	{
		dynamicGameObjects[i]->GetTransform()->Rotate(deltaTime * 0.2f, deltaTime * 0.5f, 0);
	}

	// Update the camera
	camera->Update(deltaTime);
}

// This is not running - DrawRT is
// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];
	// Clearing the render target
	{
		// Transition the back buffer from present to render target
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		float color[] = { 0.6f, 0.6f, 0.75f, 1.0f };

		// Clear the RTV
		commandList->ClearRenderTargetView(
			rtvHandles[currentSwapBuffer],
			color,
			0, 0); // No scissor rectangles

		// Clear the depth buffer, too
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, // Max depth = 1.0f
			0, // Not clearing stencil, but need a value
			0, 0); // No scissor rects
	}
	// Render
	{
		DX12Helper& dx12Helper = DX12Helper::GetInstance();

		// Set overall pipeline state
		commandList->SetPipelineState(pipelineState.Get());

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		// Set the descriptor heap for constant buffer views
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = dx12Helper.GetCBVSRVDescriptorHeap();
		commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (std::shared_ptr<GameObject> go : dynamicGameObjects)
		{
			// Get components
			std::shared_ptr<Mesh> mesh = go->GetMesh();
			std::shared_ptr<Material> material = go->GetMaterial();

			// Fill vertex shader constant buffer and apply to table (slot 0)
			{
				VertexShaderExternalData vsExData = {};
				vsExData.world = go->GetTransform()->GetWorldMatrix();
				vsExData.inverseWorldTranspose = go->GetTransform()->GetWorldInverseTransposeMatrix();
				vsExData.view = camera->GetView();
				vsExData.projection = camera->GetProjection();

				D3D12_GPU_DESCRIPTOR_HANDLE cbHandle = dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle(
					(void*)&vsExData, sizeof(VertexShaderExternalData));

				commandList->SetGraphicsRootDescriptorTable(0, cbHandle);
			}
			// Fill picel shader constant buffer and apply to table (slot 1)
			{
				PixelShaderExternalData psExtData = {};
				psExtData.cameraPosition = camera->GetTransform()->GetPosition();
				psExtData.uvScale = material->GetUVScale();
				psExtData.uvOffset = material->GetUVOffset();
				psExtData.lightCount = lightCount;
				memcpy(psExtData.lights, &lights[0], sizeof(Light) * MAX_LIGHTS);
				// Send this to a chunk of the constant buffer heap
				// and grab the GPU handle for it so we can set it for this draw
				D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS = dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle(
						(void*)(&psExtData), sizeof(PixelShaderExternalData));

				// Set this constant buffer handle
				// Note: This assumes that descriptor table 1 is the
				// place to put this particular descriptor. This
				// is based on how we set up our root signature.
				commandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);
			}

			commandList->SetPipelineState(material->GetPipelineState().Get());
			// Set the SRV descriptor handle for this material's textures
			// Note: This assumes that descriptor table 2 is for textures (as per our root sig)
			commandList->SetGraphicsRootDescriptorTable(2, material->GetFinalGPUHandleForTextures());

			D3D12_VERTEX_BUFFER_VIEW vbView = mesh->GetVertexBufferView();
			D3D12_INDEX_BUFFER_VIEW ibView = mesh->GetIndexBufferView();
			commandList->IASetVertexBuffers(0, 1, &vbView);
			commandList->IASetIndexBuffer(&ibView);

			UINT indexCount = mesh->GetIndexCount();
			commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
		}
	}
	// Present
	{
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Must occur BEFORE present
		DX12Helper::GetInstance().ExecuteCommandList();

		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= backBufferCount)
		{
			currentSwapBuffer = 0;
		}
	}
}
void Game::DrawRT(float deltaTime, float totalTime)
{
	// Grab the helper
	DX12Helper& dx12Helper = DX12Helper::GetInstance();

	// Get current buffer variables
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> currentCommandAlloc = commandAllocators[currentSwapBuffer];
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];

	// Reset allocator associated with the current buffer and set up the command list to use that allocator
	currentCommandAlloc->Reset();
	commandList->Reset(currentCommandAlloc.Get(), 0);

	// Raytrace
	RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(gameObjects);
	RaytracingHelper::GetInstance().Raytrace(
		camera,
		currentBackBuffer,
		raysPerPixel,
		maxRecursionDepth,
		directLight);

	// Finish the frame
	{
		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Wait to proceed to the next frame until the associated buffer is ready
		currentSwapBuffer = dx12Helper.SyncSwapChain(currentSwapBuffer);
	}
}