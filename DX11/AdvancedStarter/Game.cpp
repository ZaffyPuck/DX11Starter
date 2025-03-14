
#include <stdlib.h>     // For seeding random and rand()
#include <time.h>       // For grabbing time (to seed random)

#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"

#include "WICTextureLoader.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_impl_dx11.h"
#include "../../ImGui/imgui_impl_win32.h"


// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// Helper macros for making texture and shader loading code more succinct
#define LoadTexture(file, srv) CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(file).c_str(), 0, srv.GetAddressOf())
#define LoadShader(type, file) std::make_shared<type>(device.Get(), context.Get(), FixPath(file).c_str())


// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
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
	camera(0),
	sky(0),
	lightCount(0),
	showUIDemoWindow(false),
	showPointLights(false),
	ambientColor(0,0,0)
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
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object

	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
	ImGui::StyleColorsDark();

	// Asset loading and entity creation
	LoadAssetsAndCreateEntities();

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set up lights initially
	lightCount = 64;
	GenerateLights();

	// Set initial graphics API state
	//  - These settings persist until we change them
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Create the camera
	camera = std::make_shared<Camera>(
		0.0f, 0.0f, -15.0f,	// Position
		5.0f,				// Move speed (world units)
		0.002f,				// Look speed (cursor movement pixels --> radians for rotation)
		XM_PIDIV4,			// Field of view
		(float)windowWidth / windowHeight,  // Aspect ratio
		0.01f,				// Near clip
		100.0f,				// Far clip
		CameraProjectionType::Perspective);
}


// --------------------------------------------------------
// Load all assets and create materials, entities, etc.
// --------------------------------------------------------
void Game::LoadAssetsAndCreateEntities()
{
	// Load shaders using our succinct LoadShader() macro
	std::shared_ptr<SimpleVertexShader> vertexShader	= LoadShader(SimpleVertexShader, L"VertexShader.cso");
	std::shared_ptr<SimplePixelShader> pixelShader		= LoadShader(SimplePixelShader, L"PixelShader.cso");
	std::shared_ptr<SimplePixelShader> pixelShaderPBR	= LoadShader(SimplePixelShader, L"PixelShaderPBR.cso");
	std::shared_ptr<SimplePixelShader> solidColorPS		= LoadShader(SimplePixelShader, L"SolidColorPS.cso");

	std::shared_ptr<SimpleVertexShader> skyVS = LoadShader(SimpleVertexShader, L"SkyVS.cso");
	std::shared_ptr<SimplePixelShader> skyPS  = LoadShader(SimplePixelShader, L"SkyPS.cso");

	// Make the meshes
	std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device);
	std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);

	// Declare the textures we'll need
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleA,  cobbleN,  cobbleR,  cobbleM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorA,  floorN,  floorR,  floorM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintA,  paintN,  paintR,  paintM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedA,  scratchedN,  scratchedR,  scratchedM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeA,  bronzeN,  bronzeR,  bronzeM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughA,  roughN,  roughR,  roughM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodA,  woodN,  woodR,  woodM;

	// Load the textures using our succinct LoadTexture() macro
	LoadTexture(L"../../Assets/Textures/cobblestone_albedo.png", cobbleA);
	LoadTexture(L"../../Assets/Textures/cobblestone_normals.png", cobbleN);
	LoadTexture(L"../../Assets/Textures/cobblestone_roughness.png", cobbleR);
	LoadTexture(L"../../Assets/Textures/cobblestone_metal.png", cobbleM);

	LoadTexture(L"../../Assets/Textures/floor_albedo.png", floorA);
	LoadTexture(L"../../Assets/Textures/floor_normals.png", floorN);
	LoadTexture(L"../../Assets/Textures/floor_roughness.png", floorR);
	LoadTexture(L"../../Assets/Textures/floor_metal.png", floorM);

	LoadTexture(L"../../Assets/Textures/paint_albedo.png", paintA);
	LoadTexture(L"../../Assets/Textures/paint_normals.png", paintN);
	LoadTexture(L"../../Assets/Textures/paint_roughness.png", paintR);
	LoadTexture(L"../../Assets/Textures/paint_metal.png", paintM);

	LoadTexture(L"../../Assets/Textures/scratched_albedo.png", scratchedA);
	LoadTexture(L"../../Assets/Textures/scratched_normals.png", scratchedN);
	LoadTexture(L"../../Assets/Textures/scratched_roughness.png", scratchedR);
	LoadTexture(L"../../Assets/Textures/scratched_metal.png", scratchedM);

	LoadTexture(L"../../Assets/Textures/bronze_albedo.png", bronzeA);
	LoadTexture(L"../../Assets/Textures/bronze_normals.png", bronzeN);
	LoadTexture(L"../../Assets/Textures/bronze_roughness.png", bronzeR);
	LoadTexture(L"../../Assets/Textures/bronze_metal.png", bronzeM);

	LoadTexture(L"../../Assets/Textures/rough_albedo.png", roughA);
	LoadTexture(L"../../Assets/Textures/rough_normals.png", roughN);
	LoadTexture(L"../../Assets/Textures/rough_roughness.png", roughR);
	LoadTexture(L"../../Assets/Textures/rough_metal.png", roughM);

	LoadTexture(L"../../Assets/Textures/wood_albedo.png", woodA);
	LoadTexture(L"../../Assets/Textures/wood_normals.png", woodN);
	LoadTexture(L"../../Assets/Textures/wood_roughness.png", woodR);
	LoadTexture(L"../../Assets/Textures/wood_metal.png", woodM);

	// Describe and create our sampler state
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, samplerOptions.GetAddressOf());


	// Create the sky using 6 images
	sky = std::make_shared<Sky>(
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\right.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\left.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\up.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\down.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\front.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\back.png").c_str(),
		cubeMesh,
		skyVS,
		skyPS,
		samplerOptions,
		device,
		context);

	// Create non-PBR materials
	std::shared_ptr<Material> cobbleMat2x = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	cobbleMat2x->AddSampler("BasicSampler", samplerOptions);
	cobbleMat2x->AddTextureSRV("Albedo", cobbleA);
	cobbleMat2x->AddTextureSRV("NormalMap", cobbleN);
	cobbleMat2x->AddTextureSRV("RoughnessMap", cobbleR);

	std::shared_ptr<Material> cobbleMat4x = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(4, 4));
	cobbleMat4x->AddSampler("BasicSampler", samplerOptions);
	cobbleMat4x->AddTextureSRV("Albedo", cobbleA);
	cobbleMat4x->AddTextureSRV("NormalMap", cobbleN);
	cobbleMat4x->AddTextureSRV("RoughnessMap", cobbleR);

	std::shared_ptr<Material> floorMat = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	floorMat->AddSampler("BasicSampler", samplerOptions);
	floorMat->AddTextureSRV("Albedo", floorA);
	floorMat->AddTextureSRV("NormalMap", floorN);
	floorMat->AddTextureSRV("RoughnessMap", floorR);

	std::shared_ptr<Material> paintMat = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	paintMat->AddSampler("BasicSampler", samplerOptions);
	paintMat->AddTextureSRV("Albedo", paintA);
	paintMat->AddTextureSRV("NormalMap", paintN);
	paintMat->AddTextureSRV("RoughnessMap", paintR);

	std::shared_ptr<Material> scratchedMat = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	scratchedMat->AddSampler("BasicSampler", samplerOptions);
	scratchedMat->AddTextureSRV("Albedo", scratchedA);
	scratchedMat->AddTextureSRV("NormalMap", scratchedN);
	scratchedMat->AddTextureSRV("RoughnessMap", scratchedR);

	std::shared_ptr<Material> bronzeMat = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	bronzeMat->AddSampler("BasicSampler", samplerOptions);
	bronzeMat->AddTextureSRV("Albedo", bronzeA);
	bronzeMat->AddTextureSRV("NormalMap", bronzeN);
	bronzeMat->AddTextureSRV("RoughnessMap", bronzeR);

	std::shared_ptr<Material> roughMat = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	roughMat->AddSampler("BasicSampler", samplerOptions);
	roughMat->AddTextureSRV("Albedo", roughA);
	roughMat->AddTextureSRV("NormalMap", roughN);
	roughMat->AddTextureSRV("RoughnessMap", roughR);

	std::shared_ptr<Material> woodMat = std::make_shared<Material>(pixelShader, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	woodMat->AddSampler("BasicSampler", samplerOptions);
	woodMat->AddTextureSRV("Albedo", woodA);
	woodMat->AddTextureSRV("NormalMap", woodN);
	woodMat->AddTextureSRV("RoughnessMap", woodR);


	// Create PBR materials
	std::shared_ptr<Material> cobbleMat2xPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	cobbleMat2xPBR->AddSampler("BasicSampler", samplerOptions);
	cobbleMat2xPBR->AddTextureSRV("Albedo", cobbleA);
	cobbleMat2xPBR->AddTextureSRV("NormalMap", cobbleN);
	cobbleMat2xPBR->AddTextureSRV("RoughnessMap", cobbleR);
	cobbleMat2xPBR->AddTextureSRV("MetalMap", cobbleM);

	std::shared_ptr<Material> cobbleMat4xPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(4, 4));
	cobbleMat4xPBR->AddSampler("BasicSampler", samplerOptions);
	cobbleMat4xPBR->AddTextureSRV("Albedo", cobbleA);
	cobbleMat4xPBR->AddTextureSRV("NormalMap", cobbleN);
	cobbleMat4xPBR->AddTextureSRV("RoughnessMap", cobbleR);
	cobbleMat4xPBR->AddTextureSRV("MetalMap", cobbleM);

	std::shared_ptr<Material> floorMatPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	floorMatPBR->AddSampler("BasicSampler", samplerOptions);
	floorMatPBR->AddTextureSRV("Albedo", floorA);
	floorMatPBR->AddTextureSRV("NormalMap", floorN);
	floorMatPBR->AddTextureSRV("RoughnessMap", floorR);
	floorMatPBR->AddTextureSRV("MetalMap", floorM);

	std::shared_ptr<Material> paintMatPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	paintMatPBR->AddSampler("BasicSampler", samplerOptions);
	paintMatPBR->AddTextureSRV("Albedo", paintA);
	paintMatPBR->AddTextureSRV("NormalMap", paintN);
	paintMatPBR->AddTextureSRV("RoughnessMap", paintR);
	paintMatPBR->AddTextureSRV("MetalMap", paintM);

	std::shared_ptr<Material> scratchedMatPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	scratchedMatPBR->AddSampler("BasicSampler", samplerOptions);
	scratchedMatPBR->AddTextureSRV("Albedo", scratchedA);
	scratchedMatPBR->AddTextureSRV("NormalMap", scratchedN);
	scratchedMatPBR->AddTextureSRV("RoughnessMap", scratchedR);
	scratchedMatPBR->AddTextureSRV("MetalMap", scratchedM);

	std::shared_ptr<Material> bronzeMatPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	bronzeMatPBR->AddSampler("BasicSampler", samplerOptions);
	bronzeMatPBR->AddTextureSRV("Albedo", bronzeA);
	bronzeMatPBR->AddTextureSRV("NormalMap", bronzeN);
	bronzeMatPBR->AddTextureSRV("RoughnessMap", bronzeR);
	bronzeMatPBR->AddTextureSRV("MetalMap", bronzeM);

	std::shared_ptr<Material> roughMatPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	roughMatPBR->AddSampler("BasicSampler", samplerOptions);
	roughMatPBR->AddTextureSRV("Albedo", roughA);
	roughMatPBR->AddTextureSRV("NormalMap", roughN);
	roughMatPBR->AddTextureSRV("RoughnessMap", roughR);
	roughMatPBR->AddTextureSRV("MetalMap", roughM);

	std::shared_ptr<Material> woodMatPBR = std::make_shared<Material>(pixelShaderPBR, vertexShader, XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));
	woodMatPBR->AddSampler("BasicSampler", samplerOptions);
	woodMatPBR->AddTextureSRV("Albedo", woodA);
	woodMatPBR->AddTextureSRV("NormalMap", woodN);
	woodMatPBR->AddTextureSRV("RoughnessMap", woodR);
	woodMatPBR->AddTextureSRV("MetalMap", woodM);

	// Create the non-PBR entities ==============================
	float elementDepth = 10.f;
	std::shared_ptr<GameEntity> cobSphere = std::make_shared<GameEntity>(sphereMesh, cobbleMat2x);
	cobSphere->GetTransform()->SetPosition(-6, -2, elementDepth);
	cobSphere->GetTransform()->SetScale(2, 2, 2);

	std::shared_ptr<GameEntity> floorSphere = std::make_shared<GameEntity>(sphereMesh, floorMat);
	floorSphere->GetTransform()->SetPosition(-4, -2, elementDepth);
	floorSphere->GetTransform()->SetScale(2, 2, 2);

	std::shared_ptr<GameEntity> paintSphere = std::make_shared<GameEntity>(sphereMesh, paintMat);
	paintSphere->GetTransform()->SetPosition(-2, -2, elementDepth);
	paintSphere->GetTransform()->SetScale(2, 2, 2);

	std::shared_ptr<GameEntity> scratchSphere = std::make_shared<GameEntity>(sphereMesh, scratchedMat);
	scratchSphere->GetTransform()->SetPosition(0, -2, elementDepth);
	scratchSphere->GetTransform()->SetScale(2, 2, 2);

	std::shared_ptr<GameEntity> bronzeSphere = std::make_shared<GameEntity>(sphereMesh, bronzeMat);
	bronzeSphere->GetTransform()->SetPosition(2, -2, elementDepth);
	bronzeSphere->GetTransform()->SetScale(2, 2, 2);

	std::shared_ptr<GameEntity> roughSphere = std::make_shared<GameEntity>(sphereMesh, roughMat);
	roughSphere->GetTransform()->SetPosition(4, -2, elementDepth);
	roughSphere->GetTransform()->SetScale(2, 2, 2);

	std::shared_ptr<GameEntity> woodSphere = std::make_shared<GameEntity>(sphereMesh, woodMat);
	woodSphere->GetTransform()->SetPosition(6, -2, elementDepth);
	woodSphere->GetTransform()->SetScale(2, 2, 2);

	entities.push_back(cobSphere);
	entities.push_back(floorSphere);
	entities.push_back(paintSphere);
	entities.push_back(scratchSphere);
	entities.push_back(bronzeSphere);
	entities.push_back(roughSphere);
	entities.push_back(woodSphere);

	// Save assets needed for drawing point lights
	lightMesh = sphereMesh;
	lightVS = vertexShader;
	lightPS = solidColorPS;

	// Particle stuff
	// Grab loaded particle resources
	std::shared_ptr<SimpleVertexShader> particleVS = LoadShader(SimpleVertexShader, L"ParticleVS.cso");
	std::shared_ptr<SimplePixelShader> particlePS = LoadShader(SimplePixelShader, L"ParticlePS.cso");

	// Create particle materials
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fireParticleSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> twirlParticleSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> starParticleSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sparkParticleSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> animParticleSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> explParticleSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> runParticleSRV;
	LoadTexture(L"../../Assets/Particles/PNG (Transparent)/fire_01.png", fireParticleSRV);
	LoadTexture(L"../../Assets/Particles/PNG (Transparent)/twirl_03.png", twirlParticleSRV);
	LoadTexture(L"../../Assets/Particles/PNG (Transparent)/star_04.png", starParticleSRV);
	LoadTexture(L"../../Assets/Particles/PNG (Transparent)/spark_05.png", sparkParticleSRV);
	LoadTexture(L"../../Assets/Particles/flame_animated.png", animParticleSRV);
	LoadTexture(L"../../Assets/Particles/explosion_animated.png", explParticleSRV);
	LoadTexture(L"../../Assets/Particles/running_animated.png", runParticleSRV);

	std::shared_ptr<Material> fireParticle = std::make_shared<Material>(particlePS, particleVS, XMFLOAT3(1, 1, 1));
	fireParticle->AddSampler("BasicSampler", samplerOptions);
	fireParticle->AddTextureSRV("Particle", fireParticleSRV);

	std::shared_ptr<Material> twirlParticle = std::make_shared<Material>(particlePS, particleVS, XMFLOAT3(1, 1, 1));
	twirlParticle->AddSampler("BasicSampler", samplerOptions);
	twirlParticle->AddTextureSRV("Particle", twirlParticleSRV);

	std::shared_ptr<Material> starParticle = std::make_shared<Material>(particlePS, particleVS, XMFLOAT3(1, 1, 1));
	starParticle->AddSampler("BasicSampler", samplerOptions);
	starParticle->AddTextureSRV("Particle", starParticleSRV);

	std::shared_ptr<Material> animParticle = std::make_shared<Material>(particlePS, particleVS, XMFLOAT3(1, 1, 1));
	animParticle->AddSampler("BasicSampler", samplerOptions);
	animParticle->AddTextureSRV("Particle", animParticleSRV);

	std::shared_ptr<Material> explParticle = std::make_shared<Material>(particlePS, particleVS, XMFLOAT3(1, 1, 1));
	explParticle->AddSampler("BasicSampler", samplerOptions);
	explParticle->AddTextureSRV("Particle", explParticleSRV);

	std::shared_ptr<Material> runParticle = std::make_shared<Material>(particlePS, particleVS, XMFLOAT3(1, 1, 1));
	runParticle->AddSampler("BasicSampler", samplerOptions);
	runParticle->AddTextureSRV("Particle", runParticleSRV);

	std::shared_ptr<Material> sparkParticle = std::make_shared<Material>(particlePS, particleVS, XMFLOAT3(1, 1, 1));
	sparkParticle->AddSampler("BasicSampler", samplerOptions);
	sparkParticle->AddTextureSRV("Particle", sparkParticleSRV);

	// Particle states ====

	// A depth state for the particles
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Turns off depth writing
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&dsDesc, particleDepthState.GetAddressOf());

	// Blend for particles (additive)
	D3D11_BLEND_DESC blend = {};
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;
	blend.RenderTarget[0].BlendEnable = true;
	blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // Still respect pixel shader output alpha
	blend.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&blend, particleBlendState.GetAddressOf());

	// Debug rasterizer state for particles
	D3D11_RASTERIZER_DESC rd = {};
	rd.CullMode = D3D11_CULL_BACK;
	rd.DepthClipEnable = true;
	rd.FillMode = D3D11_FILL_WIREFRAME;
	device->CreateRasterizerState(&rd, particleDebugRasterState.GetAddressOf());

	// PE1 - Single Waterfall
	emitters.push_back(std::make_shared<Emitter>(
		device,			// Device
		fireParticle,	// Material
		100,			// Particle capacity
		3.f,			// Particle lifetime
		30,				// Particles per second
		0.1f,			// Start size
		2.0f,			// End size
		false,			// lock y axis
		XMFLOAT4(0.1f, 0.1f, 1.0f, 0.9f),	// Start color
		XMFLOAT4(0.5f, 0.5f, 0.5f, 0),		// End color
		XMFLOAT3(-4, -1, 0),				// Emitter position
		XMFLOAT3(0.1f, 0.1f, 0.1f),		// Position randomness range
		XMFLOAT2(-2, 2),				// rotationStartMinMax
		XMFLOAT2(-2, 2),				// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(0, 0, 0),				// Start velocity
		XMFLOAT3(0.1f, 0.1f, 0.1f),		// Velocity randomness range
		XMFLOAT3(0, -1, 0)));			// Constant acceleration

	// PE2 - Portal
	emitters.push_back(std::make_shared<Emitter>(
		device,
		twirlParticle,
		45,								// Max particles
		2.0f,							// Particle lifetime
		20,								// Particles per second
		3.0f,							// Start size
		1.0f,							// End size
		false,
		XMFLOAT4(0.2f, 0.1f, 0.1f, 0.1f),// Start color
		XMFLOAT4(0.2f, 0.7f, 0.1f, 1.0f),// End color
		XMFLOAT3(-2.f, 2.5f, 0),		// Emitter position
		XMFLOAT3(0, 0, 0),				// Position randomness range
		XMFLOAT2(-5, 10),
		XMFLOAT2(-10, 5),			// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(0, 0, 0),				// Start velocity
		XMFLOAT3(0.1f, 0.1f, 0.1f),				// Velocity randomness range
		XMFLOAT3(0, 0, 0)));			// Constant acceleration

	// PE3
	emitters.push_back(std::make_shared<Emitter>(
		device,
		starParticle,
		250,							// Max particles
		2.0f,							// Particle lifetime
		100,							// Particles per second
		2.0f,							// Start size
		0.0f,							// End size
		false,
		XMFLOAT4(0.1f, 0.2f, 0.5f, 0.0f),// Start color
		XMFLOAT4(0.1f, 0.1f, 0.3f, 3.0f),// End color (ending with high alpha so we hit 1.0 sooner)
		XMFLOAT3(-2.5f, -1, 0),			// Emitter position
		XMFLOAT3(1, 0, 1),				// Position randomness range
		XMFLOAT2(0, 0),
		XMFLOAT2(-3, 3),			// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(0, 0, 0),				// Start velocity
		XMFLOAT3(0.1f, 0, 0.1f),		// Velocity randomness range
		XMFLOAT3(0, -2, 0)));			// Constant acceleration


	// PE4
	emitters.push_back(std::make_shared<Emitter>(
		device,
		animParticle,
		5,						// Max particles
		2.0f,					// Particle lifetime
		2,						// Particles per second
		2.0f,					// Start size
		2.0f,					// End size
		false,
		XMFLOAT4(1, 1, 1, 1),	// Start color
		XMFLOAT4(1, 1, 1, 0),	// End color
		XMFLOAT3(2, -2, 0),		// Emitter position
		XMFLOAT3(0, 0, 0),		// Position randomness range
		XMFLOAT2(-2, 2),
		XMFLOAT2(-2, 2),	// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(0, 0, 0),		// Start velocity
		XMFLOAT3(0, 0, 0),		// Velocity randomness range
		XMFLOAT3(0, 0, 0),		// Constant acceleration
		8,
		8));

	// PE5 - Explosion
	emitters.push_back(std::make_shared<Emitter>(
		device,
		explParticle,
		1,						// Max particles
		0.5f,					// Particle lifetime
		1,						// Particles per second
		1.0f,					// Start size
		3.0f,					// End size
		false,
		XMFLOAT4(1, 1, 1, 1),	// Start color
		XMFLOAT4(1, 1, 1, 0.3),	// End color
		XMFLOAT3(5, -2, 0),		// Emitter position
		XMFLOAT3(0, 0, 0),		// Position randomness range
		XMFLOAT2(0, 0),
		XMFLOAT2(0, 0),	// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(0, 0, 0),		// Start velocity
		XMFLOAT3(0, 0, 0),		// Velocity randomness range
		XMFLOAT3(0, 0, 0),		// Constant acceleration
		6,
		6));

	// PE6 - running
	emitters.push_back(std::make_shared<Emitter>(
		device,
		runParticle,
		2,						// Max particles
		1.f,					// Particle lifetime
		1,						// Particles per second
		1.0f,					// Start size
		1.0f,					// End size
		false,
		XMFLOAT4(1, 1, 1, 1),	// Start color
		XMFLOAT4(1, 1, 1, 1),	// End color
		XMFLOAT3(7, -2, 0),		// Emitter position
		XMFLOAT3(0, 0, 0),		// Position randomness range
		XMFLOAT2(0, 0),
		XMFLOAT2(0, 0),	// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(0, 0, 0),		// Start velocity
		XMFLOAT3(0, 0, 0),		// Velocity randomness range
		XMFLOAT3(0, 0, 0),		// Constant acceleration
		5, 2, 2.f));

	// PE6 - running
	emitters.push_back(std::make_shared<Emitter>(
		device,
		sparkParticle,
		50,						// Max particles
		2.f,					// Particle lifetime
		50,						// Particles per second
		2.5f,					// Start size
		2.0f,					// End size
		false,
		XMFLOAT4(1, 1, 1, 1),	// Start color
		XMFLOAT4(0.3f, 0.3f, 1.f, 0.5),	// End color
		XMFLOAT3(2, 3, 0),		// Emitter position
		XMFLOAT3(0.1f, 0.1f, 0.1f),		// Position randomness range
		XMFLOAT2(-0.2f, 0.2f),
		XMFLOAT2(-0.2f, 0.2f),	// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(0, 0, 0),		// Start velocity
		XMFLOAT3(0, 0, 0),		// Velocity randomness range
		XMFLOAT3(0, 0, 0)));		// Constant acceleration

	// PE6 - shot
	emitters.push_back(std::make_shared<Emitter>(
		device,
		fireParticle,
		3,						// Max particles
		1.f,					// Particle lifetime
		2,						// Particles per second
		1.0f,					// Start size
		0.5f,					// End size
		false,
		XMFLOAT4(1, 0, 0, 1),	// Start color
		XMFLOAT4(0, 0, 0, 1),	// End color
		XMFLOAT3(5, 2, 0),		// Emitter position
		XMFLOAT3(0.1f, 0.1f, 0.1f),		// Position randomness range
		XMFLOAT2(-0.2f, 0.2f),
		XMFLOAT2(-0.2f, 0.2f),	// Random rotation ranges (startMin, startMax, endMin, endMax)
		XMFLOAT3(60, 0, 0),		// Start velocity
		XMFLOAT3(0, 0, 0),		// Velocity randomness range
		XMFLOAT3(-120, -1, 0)));		// Constant acceleration

	// PE6 - shot
	emitters.push_back(std::make_shared<Emitter>(
		device,
		fireParticle,
		1000,						// Max particles
		3.f,					// Particle lifetime
		175,						// Particles per second
		0.1f,					// Start size
		0.5f,					// End size
		false,
		XMFLOAT4(1, 1, 0, 1),	// Start color
		XMFLOAT4(1, 0, 1, 0.1),	// End color
		XMFLOAT3(-4, 0, 0),		// Emitter position
		XMFLOAT3(4.f, 0.1f, 2.f),		// Position randomness range
		XMFLOAT2(-1.f, 1.f),	// rot start angle (min/max)
		XMFLOAT2(-1.f, 1.f),	// rot end angle (min/max)
		XMFLOAT3(0, 2, 0),		// Start velocity
		XMFLOAT3(0.5, 0.5, 0.5),		// Velocity randomness range
		XMFLOAT3(0, -1, 0)));		// Constant acceleration
}


// --------------------------------------------------------
// Generates the lights in the scene: 3 directional lights
// and many random point lights.
// --------------------------------------------------------
void Game::GenerateLights()
{
	// Reset
	lights.clear();

	// Setup directional lights
	Light dir1 = {};
	dir1.Type = LIGHT_TYPE_DIRECTIONAL;
	dir1.Direction = XMFLOAT3(1, -1, 1);
	dir1.Color = XMFLOAT3(0.8f, 0.8f, 0.8f);
	dir1.Intensity = 1.0f;

	Light dir2 = {};
	dir2.Type = LIGHT_TYPE_DIRECTIONAL;
	dir2.Direction = XMFLOAT3(-1, -0.25f, 0);
	dir2.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir2.Intensity = 1.0f;

	Light dir3 = {};
	dir3.Type = LIGHT_TYPE_DIRECTIONAL;
	dir3.Direction = XMFLOAT3(0, -1, 1);
	dir3.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir3.Intensity = 1.0f;

	// Add light to the list
	lights.push_back(dir1);
	lights.push_back(dir2);
	lights.push_back(dir3);

	// Create the rest of the lights
	while (lights.size() < MAX_LIGHTS)
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-10.0f, 10.0f), RandomRange(-5.0f, 5.0f), RandomRange(-10.0f, 10.0f));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(5.0f, 10.0f);
		point.Intensity = RandomRange(0.1f, 3.0f);

		// Add to the list
		lights.push_back(point);
	}

}

// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update our projection matrix to match the new aspect ratio
	if (camera)
		camera->UpdateProjectionMatrix(this->windowWidth / (float)this->windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Set up the new frame for the UI, then build
	// this frame's interface.  Note that the building
	// of the UI could happen at any point during update.
	UINewFrame(deltaTime);
	BuildUI();

	// Update the camera
	camera->Update(deltaTime);

	for (auto& emitter : emitters)
	{
		emitter->Update(deltaTime, totalTime);
	}

	// Check individual input
	Input& input = Input::GetInstance();
	if (input.KeyDown(VK_ESCAPE)) Quit();
	if (input.KeyPress(VK_TAB)) GenerateLights();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	// Draw all of the entities
	for (auto& ge : entities)
	{
		// Set the "per frame" data
		// Note that this should literally be set once PER FRAME, before
		// the draw loop, but we're currently setting it per entity since
		// we are just using whichever shader the current entity has.
		// Inefficient!!!
		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetData("lights", (void*)(&lights[0]), sizeof(Light) * lightCount);
		ps->SetInt("lightCount", lightCount);
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
		ps->CopyBufferData("perFrame");

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the light sources?
	if(showPointLights)
		DrawPointLights();

	// Draw the sky
	sky->Draw(camera);

	// Particle drawing
	{
		// Particle states
		context->OMSetBlendState(particleBlendState.Get(), 0, 0xffffffff);	// Additive blending
		context->OMSetDepthStencilState(particleDepthState.Get(), 0);		// No depth WRITING

		// Draw all of the emitters
		for (auto& e : emitters)
		{
			e->Draw(context, camera, totalTime);
		}

		// Reset to default states for next frame
		context->OMSetBlendState(0, 0, 0xffffffff);
		context->OMSetDepthStencilState(0, 0);
		context->RSSetState(0);
	}

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Draw the UI after everything else
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}

// --------------------------------------------------------
// Draws the point lights as solid color spheres
// --------------------------------------------------------
void Game::DrawPointLights()
{
	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", camera->GetView());
	lightVS->SetMatrix4x4("projection", camera->GetProjection());

	for (int i = 0; i < lightCount; i++)
	{
		Light light = lights[i];

		// Only drawing points, so skip others
		if (light.Type != LIGHT_TYPE_POINT)
			continue;

		// Calc quick scale based on range
		float scale = light.Range / 20.0f;

		// Make the transform for this light
		XMMATRIX rotMat = XMMatrixIdentity();
		XMMATRIX scaleMat = XMMatrixScaling(scale, scale, scale);
		XMMATRIX transMat = XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);
		XMMATRIX worldMat = scaleMat * rotMat * transMat;

		XMFLOAT4X4 world;
		XMFLOAT4X4 worldInvTrans;
		XMStoreFloat4x4(&world, worldMat);
		XMStoreFloat4x4(&worldInvTrans, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		// Set up the world matrix for this light
		lightVS->SetMatrix4x4("world", world);
		lightVS->SetMatrix4x4("worldInverseTranspose", worldInvTrans);

		// Set up the pixel shader data
		XMFLOAT3 finalColor = light.Color;
		finalColor.x *= light.Intensity;
		finalColor.y *= light.Intensity;
		finalColor.z *= light.Intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		lightMesh->SetBuffersAndDraw(context);
	}

}

// --------------------------------------------------------
// Prepares a new frame for the UI, feeding it fresh
// input and time information for this new frame.
// --------------------------------------------------------
void Game::UINewFrame(float deltaTime)
{
	// Get a reference to our custom input manager
	Input& input = Input::GetInstance();

	// Reset input manager's gui state so we don�t
	// taint our own input
	input.SetKeyboardCapture(false);
	input.SetMouseCapture(false);

	// Feed fresh input data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;

	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new input capture
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);
}

// --------------------------------------------------------
// Builds the UI for the current frame
// --------------------------------------------------------
void Game::BuildUI()
{
	// Should we show the built-in demo window?
	if (showUIDemoWindow)
	{
		ImGui::ShowDemoWindow();
	}

	// Actually build our custom UI, starting with a window
	ImGui::Begin("Inspector");
	{
		// Set a specific amount of space for widget labels
		ImGui::PushItemWidth(-160); // Negative value sets label width

		// === Overall details ===
		if (ImGui::TreeNode("App Details"))
		{
			ImGui::Spacing();
			ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
			ImGui::Text("Window Client Size: %dx%d", windowWidth, windowHeight);

			ImGui::Spacing();
			ImGui::Text("Scene Details");
			ImGui::Text("Top Row:");    ImGui::SameLine(125); ImGui::Text("PBR Materials");
			ImGui::Text("Bottom Row:"); ImGui::SameLine(125); ImGui::Text("Non-PBR Materials");

			// Should we show the demo window?
			ImGui::Spacing();
			if (ImGui::Button(showUIDemoWindow ? "Hide ImGui Demo Window" : "Show ImGui Demo Window"))
				showUIDemoWindow = !showUIDemoWindow;

			ImGui::Spacing();

			// Finalize the tree node
			ImGui::TreePop();
		}

		// === Controls ===
		if (ImGui::TreeNode("Controls"))
		{
			ImGui::Spacing();
			ImGui::Text("(WASD, X, Space)");    ImGui::SameLine(175); ImGui::Text("Move camera");
			ImGui::Text("(Left Click & Drag)"); ImGui::SameLine(175); ImGui::Text("Rotate camera");
			ImGui::Text("(Left Shift)");        ImGui::SameLine(175); ImGui::Text("Hold to speed up camera");
			ImGui::Text("(Left Ctrl)");         ImGui::SameLine(175); ImGui::Text("Hold to slow down camera");
			ImGui::Text("(TAB)");               ImGui::SameLine(175); ImGui::Text("Randomize lights");
			ImGui::Spacing();

			// Finalize the tree node
			ImGui::TreePop();
		}

		// === Camera details ===
		if (ImGui::TreeNode("Camera"))
		{
			// Show UI for current camera
			CameraUI(camera);

			// Finalize the tree node
			ImGui::TreePop();
		}

		// === Entities ===
		if (ImGui::TreeNode("Scene Entities"))
		{
			// Loop and show the details for each entity
			for (int i = 0; i < entities.size(); i++)
			{
				// New node for each entity
				// Note the use of PushID(), so that each tree node and its widgets
				// have unique internal IDs in the ImGui system
				ImGui::PushID(i);
				if (ImGui::TreeNode("Entity Node", "Entity %d", i))
				{
					// Build UI for one entity at a time
					EntityUI(entities[i]);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			// Finalize the tree node
			ImGui::TreePop();
		}

		// === Lights ===
		if (ImGui::TreeNode("Lights"))
		{
			// Light details
			ImGui::Spacing();
			ImGui::SliderInt("Light Count", &lightCount, 0, MAX_LIGHTS);
			ImGui::Checkbox("Show Point Lights", &showPointLights);
			ImGui::Spacing();

			// Loop and show the details for each entity
			for (int i = 0; i < lightCount; i++)
			{
				// Name of this light based on type
				std::string lightName = "Light %d";
				switch (lights[i].Type)
				{
				case LIGHT_TYPE_DIRECTIONAL: lightName += " (Directional)"; break;
				case LIGHT_TYPE_POINT: lightName += " (Point)"; break;
				case LIGHT_TYPE_SPOT: lightName += " (Spot)"; break;
				}

				// New node for each light
				// Note the use of PushID(), so that each tree node and its widgets
				// have unique internal IDs in the ImGui system
				ImGui::PushID(i);
				if (ImGui::TreeNode("Light Node", lightName.c_str(), i))
				{
					// Build UI for one entity at a time
					LightUI(lights[i]);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			// Finalize the tree node
			ImGui::TreePop();
		}
	}
	ImGui::End();
}

// --------------------------------------------------------
// Builds the UI for a single camera
// --------------------------------------------------------
void Game::CameraUI(std::shared_ptr<Camera> cam)
{
	ImGui::Spacing();

	// Transform details
	XMFLOAT3 pos = cam->GetTransform()->GetPosition();
	XMFLOAT3 rot = cam->GetTransform()->GetPitchYawRoll();

	if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
		cam->GetTransform()->SetPosition(pos);
	if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f))
		cam->GetTransform()->SetRotation(rot);
	ImGui::Spacing();

	// Clip planes
	float nearClip = cam->GetNearClip();
	float farClip = cam->GetFarClip();
	if (ImGui::DragFloat("Near Clip Distance", &nearClip, 0.01f, 0.001f, 1.0f))
		cam->SetNearClip(nearClip);
	if (ImGui::DragFloat("Far Clip Distance", &farClip, 1.0f, 10.0f, 1000.0f))
		cam->SetFarClip(farClip);

	// Projection type
	CameraProjectionType projType = cam->GetProjectionType();
	int typeIndex = (int)projType;
	if (ImGui::Combo("Projection Type", &typeIndex, "Perspective\0Orthographic"))
	{
		projType = (CameraProjectionType)typeIndex;
		cam->SetProjectionType(projType);
	}

	// Projection details
	if (projType == CameraProjectionType::Perspective)
	{
		// Convert field of view to degrees for UI
		float fov = cam->GetFieldOfView() * 180.0f / XM_PI;
		if (ImGui::SliderFloat("Field of View (Degrees)", &fov, 0.01f, 180.0f))
			cam->SetFieldOfView(fov * XM_PI / 180.0f); // Back to radians
	}
	else if (projType == CameraProjectionType::Orthographic)
	{
		float wid = cam->GetOrthographicWidth();
		if (ImGui::SliderFloat("Orthographic Width", &wid, 1.0f, 10.0f))
			cam->SetOrthographicWidth(wid);
	}

	ImGui::Spacing();
}

// --------------------------------------------------------
// Builds the UI for a single entity
// --------------------------------------------------------
void Game::EntityUI(std::shared_ptr<GameEntity> entity)
{
	ImGui::Spacing();

	// Transform details
	Transform* trans = entity->GetTransform();
	XMFLOAT3 pos = trans->GetPosition();
	XMFLOAT3 rot = trans->GetPitchYawRoll();
	XMFLOAT3 sca = trans->GetScale();

	if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) trans->SetPosition(pos);
	if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f)) trans->SetRotation(rot);
	if (ImGui::DragFloat3("Scale", &sca.x, 0.01f)) trans->SetScale(sca);

	// Mesh details
	ImGui::Spacing();
	ImGui::Text("Mesh Index Count: %d", entity->GetMesh()->GetIndexCount());

	ImGui::Spacing();
}

// --------------------------------------------------------
// Builds the UI for a single light
// --------------------------------------------------------
void Game::LightUI(Light& light)
{
	// Light type
	if (ImGui::RadioButton("Directional", light.Type == LIGHT_TYPE_DIRECTIONAL))
	{
		light.Type = LIGHT_TYPE_DIRECTIONAL;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Point", light.Type == LIGHT_TYPE_POINT))
	{
		light.Type = LIGHT_TYPE_POINT;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Spot", light.Type == LIGHT_TYPE_SPOT))
	{
		light.Type = LIGHT_TYPE_SPOT;
	}

	// Direction
	if (light.Type == LIGHT_TYPE_DIRECTIONAL || light.Type == LIGHT_TYPE_SPOT)
	{
		ImGui::DragFloat3("Direction", &light.Direction.x, 0.1f);

		// Normalize the direction
		XMVECTOR dirNorm = XMVector3Normalize(XMLoadFloat3(&light.Direction));
		XMStoreFloat3(&light.Direction, dirNorm);
	}

	// Position & Range
	if (light.Type == LIGHT_TYPE_POINT || light.Type == LIGHT_TYPE_SPOT)
	{
		ImGui::DragFloat3("Position", &light.Position.x, 0.1f);
		ImGui::SliderFloat("Range", &light.Range, 0.1f, 100.0f);
	}

	// Spot falloff
	if (light.Type == LIGHT_TYPE_SPOT)
	{
		ImGui::SliderFloat("Spot Falloff", &light.SpotFalloff, 0.1f, 128.0f);
	}

	// Color details
	ImGui::ColorEdit3("Color", &light.Color.x);
	ImGui::SliderFloat("Intensity", &light.Intensity, 0.0f, 10.0f);
}