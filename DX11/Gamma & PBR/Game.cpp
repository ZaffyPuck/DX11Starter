#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"
#include <iostream>
//extern std::ostream cout; // or this

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
// hInstance - the application's OS-level handle (unique ID)
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true),				// Show extra stats (fps) in title bar?
lightCount(8),
gammaCorrection(true),
useSpecularMap(false),
useAlbedoTexture(true),
useMetalMap(true),
useNormalMap(true),
useRoughnessMap(false)
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	IGDelete();
}

// Called once per program, after Direct3D and the window are initialized but before the game loop.
void Game::Init()
{
	IGInit();				// Init ImGUI
	LoadShaders();			// Init Shaders
	LoadTextures();			// Init Textures
	LoadObjects();			// Init Objects
	//CreateGeometry();		// Init Geometry
	CreateEntities();		// Init Entities
	InitSkyBox();
	CreateLights();
	InitGraphicsState();	// Init Graphics
	CreateCameras();		// Init Cameras
}
/// <summary>
/// Loads shaders from compiled shader object (.cso) files
/// </summary>
void Game::LoadShaders()
{
	// Config
	//ISimpleShader::ReportErrors = true;
	//ISimpleShader::ReportWarnings = true;

	// Create & Store vertex shaders
	std::shared_ptr<SimpleVertexShader> vertexShader = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"VertexShader.cso").c_str());
	std::shared_ptr<SimpleVertexShader> skyVS = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"SkyVS.cso").c_str());
	std::shared_ptr<SimpleVertexShader> normalMapVS = std::make_shared<SimpleVertexShader>(
		device, context, FixPath(L"NormalMapVS.cso").c_str());
	vertexShaders.push_back(vertexShader);
	vertexShaders.push_back(skyVS);
	vertexShaders.push_back(normalMapVS);

	// Create & Store vertex shaders
	std::shared_ptr<SimplePixelShader> pixelShader = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"PixelShader.cso").c_str());
	std::shared_ptr<SimplePixelShader> skyPS = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"SkyPS.cso").c_str());
	std::shared_ptr<SimplePixelShader> normalMapPS = std::make_shared<SimplePixelShader>(
		device, context, FixPath(L"NormalMapPS.cso").c_str());
	pixelShaders.push_back(pixelShader);
	pixelShaders.push_back(skyPS);
	pixelShaders.push_back(normalMapPS);
}
/// <summary>
/// Loads textures, samples them
/// </summary>
void Game::LoadTextures()
{
	// Build sampler description with texture sampling options 
	D3D11_SAMPLER_DESC samplerDesc = {}; // zeros all values
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 16; // needed if filter is anisotropic
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT sameplerResult = device->CreateSamplerState(&samplerDesc, sampler.GetAddressOf()); // Create sampler state

	// Declare the textures we'll need
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleA, cobbleN, cobbleR, cobbleM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorA, floorN, floorR, floorM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintA, paintN, paintR, paintM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedA, scratchedN, scratchedR, scratchedM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeA, bronzeN, bronzeR, bronzeM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughA, roughN, roughR, roughM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodA, woodN, woodR, woodM;

#define LoadTexture(path, srv) CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(path).c_str(), 0, srv.GetAddressOf());

	LoadTexture(L"../../External/Textures/PBR/cobblestone_albedo.png", cobbleA);
	LoadTexture(L"../../External/Textures/PBR/cobblestone_normals.png", cobbleN);
	LoadTexture(L"../../External/Textures/PBR/cobblestone_roughness.png", cobbleR);
	LoadTexture(L"../../External/Textures/PBR/cobblestone_metal.png", cobbleM);

	LoadTexture(L"../../External/Textures/PBR/floor_albedo.png", floorA);
	LoadTexture(L"../../External/Textures/PBR/floor_normals.png", floorN);
	LoadTexture(L"../../External/Textures/PBR/floor_roughness.png", floorR);
	LoadTexture(L"../../External/Textures/PBR/floor_metal.png", floorM);

	LoadTexture(L"../../External/Textures/PBR/paint_albedo.png", paintA);
	LoadTexture(L"../../External/Textures/PBR/paint_normals.png", paintN);
	LoadTexture(L"../../External/Textures/PBR/paint_roughness.png", paintR);
	LoadTexture(L"../../External/Textures/PBR/paint_metal.png", paintM);

	LoadTexture(L"../../External/Textures/PBR/scratched_albedo.png", scratchedA);
	LoadTexture(L"../../External/Textures/PBR/scratched_normals.png", scratchedN);
	LoadTexture(L"../../External/Textures/PBR/scratched_roughness.png", scratchedR);
	LoadTexture(L"../../External/Textures/PBR/scratched_metal.png", scratchedM);

	LoadTexture(L"../../External/Textures/PBR/bronze_albedo.png", bronzeA);
	LoadTexture(L"../../External/Textures/PBR/bronze_normals.png", bronzeN);
	LoadTexture(L"../../External/Textures/PBR/bronze_roughness.png", bronzeR);
	LoadTexture(L"../../External/Textures/PBR/bronze_metal.png", bronzeM);

	LoadTexture(L"../../External/Textures/PBR/rough_albedo.png", roughA);
	LoadTexture(L"../../External/Textures/PBR/rough_normals.png", roughN);
	LoadTexture(L"../../External/Textures/PBR/rough_roughness.png", roughR);
	LoadTexture(L"../../External/Textures/PBR/rough_metal.png", roughM);

	LoadTexture(L"../../External/Textures/PBR/wood_albedo.png", woodA);
	LoadTexture(L"../../External/Textures/PBR/wood_normals.png", woodN);
	LoadTexture(L"../../External/Textures/PBR/wood_roughness.png", woodR);
	LoadTexture(L"../../External/Textures/PBR/wood_metal.png", woodM);

	// Load textures
	std::shared_ptr<Material> cobbleMat2x = std::make_shared<Material>(
		white3, vertexShaders[0], pixelShaders[0], XMFLOAT2(2, 2));
	cobbleMat2x->AddSampler("BasicSampler", sampler);
	cobbleMat2x->AddTextureSRV("Albedo", cobbleA);
	cobbleMat2x->AddTextureSRV("NormalMap", cobbleN);
	cobbleMat2x->AddTextureSRV("RoughnessMap", cobbleR);
	cobbleMat2x->AddTextureSRV("MetalMap", cobbleM);

	std::shared_ptr<Material> cobbleMat4x = std::make_shared<Material>(
		white3, vertexShaders[0], pixelShaders[0], XMFLOAT2(4, 4));
	cobbleMat4x->AddSampler("BasicSampler", sampler);
	cobbleMat4x->AddTextureSRV("Albedo", cobbleA);
	cobbleMat4x->AddTextureSRV("NormalMap", cobbleN);
	cobbleMat4x->AddTextureSRV("RoughnessMap", cobbleR);
	cobbleMat4x->AddTextureSRV("MetalMap", cobbleM);

	std::shared_ptr<Material> floorMat = std::make_shared<Material>(
		white3, vertexShaders[0],pixelShaders[0], XMFLOAT2(2, 2));
	floorMat->AddSampler("BasicSampler", sampler);
	floorMat->AddTextureSRV("Albedo", floorA);
	floorMat->AddTextureSRV("NormalMap", floorN);
	floorMat->AddTextureSRV("RoughnessMap", floorR);
	floorMat->AddTextureSRV("MetalMap", floorM);

	std::shared_ptr<Material> paintMat = std::make_shared<Material>(
		white3, vertexShaders[0],pixelShaders[0], XMFLOAT2(2, 2));
	paintMat->AddSampler("BasicSampler", sampler);
	paintMat->AddTextureSRV("Albedo", paintA);
	paintMat->AddTextureSRV("NormalMap", paintN);
	paintMat->AddTextureSRV("RoughnessMap", paintR);
	paintMat->AddTextureSRV("MetalMap", paintM);

	std::shared_ptr<Material> scratchedMat = std::make_shared<Material>(
		white3, vertexShaders[0],pixelShaders[0], XMFLOAT2(2, 2));
	scratchedMat->AddSampler("BasicSampler", sampler);
	scratchedMat->AddTextureSRV("Albedo", scratchedA);
	scratchedMat->AddTextureSRV("NormalMap", scratchedN);
	scratchedMat->AddTextureSRV("RoughnessMap", scratchedR);
	scratchedMat->AddTextureSRV("MetalMap", scratchedM);

	std::shared_ptr<Material> bronzeMat = std::make_shared<Material>(
		white3, vertexShaders[0],pixelShaders[0], XMFLOAT2(2, 2));
	bronzeMat->AddSampler("BasicSampler", sampler);
	bronzeMat->AddTextureSRV("Albedo", bronzeA);
	bronzeMat->AddTextureSRV("NormalMap", bronzeN);
	bronzeMat->AddTextureSRV("RoughnessMap", bronzeR);
	bronzeMat->AddTextureSRV("MetalMap", bronzeM);

	std::shared_ptr<Material> roughMat = std::make_shared<Material>(
		white3, vertexShaders[0],pixelShaders[0], XMFLOAT2(2, 2));
	roughMat->AddSampler("BasicSampler", sampler);
	roughMat->AddTextureSRV("Albedo", roughA);
	roughMat->AddTextureSRV("NormalMap", roughN);
	roughMat->AddTextureSRV("RoughnessMap", roughR);
	roughMat->AddTextureSRV("MetalMap", roughM);

	std::shared_ptr<Material> woodMat = std::make_shared<Material>(
		white3, vertexShaders[0],pixelShaders[0],  XMFLOAT2(2, 2));
	woodMat->AddSampler("BasicSampler", sampler);
	woodMat->AddTextureSRV("Albedo", woodA);
	woodMat->AddTextureSRV("NormalMap", woodN);
	woodMat->AddTextureSRV("RoughnessMap", woodR);
	woodMat->AddTextureSRV("MetalMap", woodM);

	materials.insert(materials.end(), { cobbleMat2x, cobbleMat4x, floorMat, paintMat, scratchedMat, bronzeMat, roughMat, woodMat });
}
void Game::LoadObjects()
{
	std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(FixPath(L"../../External//Models/cube.obj").c_str(), device);
	std::shared_ptr<Mesh> cylinderMesh = std::make_shared<Mesh>(FixPath(L"../../External/Models/cylinder.obj").c_str(), device);
	std::shared_ptr<Mesh> helixMesh = std::make_shared<Mesh>(FixPath(L"../../External/Models/helix.obj").c_str(), device);
	std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(FixPath(L"../../External/Models/sphere.obj").c_str(), device);
	std::shared_ptr<Mesh> torusMesh = std::make_shared<Mesh>(FixPath(L"../../External/Models/torus.obj").c_str(), device);
	std::shared_ptr<Mesh> quadMesh = std::make_shared<Mesh>(FixPath(L"../../External/Models/quad.obj").c_str(), device);
	std::shared_ptr<Mesh> quad2sidedMesh = std::make_shared<Mesh>(FixPath(L"../../External/Models/quad_double_sided.obj").c_str(), device);
	meshes.push_back(cubeMesh);
	meshes.push_back(cylinderMesh);
	meshes.push_back(helixMesh);
	meshes.push_back(sphereMesh);
	meshes.push_back(torusMesh);
	meshes.push_back(quadMesh);
	meshes.push_back(quad2sidedMesh);
	//meshes.insert(meshes.end(), { cubeMesh, cylinderMesh, helixMesh, sphereMesh, torusMesh, quadMesh, quad2sidedMesh });

	lightMesh = sphereMesh;
}
void Game::InitSkyBox()
{
	// Create the sky
	sky = std::make_shared<Sky>(meshes[0], sampler, device, context, vertexShaders[1], pixelShaders[1],
		FixPath(L"../../External/Skies/Clouds Blue/right.png").c_str(),
		FixPath(L"../../External/Skies/Clouds Blue/left.png").c_str(),
		FixPath(L"../../External/Skies/Clouds Blue/up.png").c_str(),
		FixPath(L"../../External/Skies/Clouds Blue/down.png").c_str(),
		FixPath(L"../../External/Skies/Clouds Blue/front.png").c_str(),
		FixPath(L"../../External/Skies/Clouds Blue/back.png").c_str());
}
void Game::CreateEntities()
{
	std::shared_ptr<Entity> floor = std::make_shared<Entity>(meshes[0], materials[1]);
	floor->GetTransform()->SetScale(20, 1, 20);
	entities.push_back(floor);

	for (int i = 0; i < materials.size(); i++)
	{
		entities.push_back(std::make_shared<Entity>(meshes[3], materials[i]));
	}

	float x = 0.0f;
	float y = 0.0f;
	float deltaX = 2.0f;
	float deltaY = 0.0f;
	float offsetX = -6.0f;
	float offsetY = 4.0f;
	for (size_t i = 1; i < entities.size(); i++)
	{
		entities[i]->GetTransform()->SetPosition(x + offsetX, y + offsetY, 0.0f);
		x += deltaX;
		y += deltaY;
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedoSRV = CreateSolidColorTextureSRV(2, 2, XMFLOAT4(1, 1, 1, 1));
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal0SRV = CreateSolidColorTextureSRV(2, 2, XMFLOAT4(0, 0, 0, 1));
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metal1SRV = CreateSolidColorTextureSRV(2, 2, XMFLOAT4(1, 1, 1, 1));

	for (int i = 0; i <= 10; i++)
	{
		// Roughness value for this entity
		float r = i / 10.0f;

		// Create textures
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughSRV = CreateSolidColorTextureSRV(2, 2, XMFLOAT4(r, r, r, 1));
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalSRV = CreateSolidColorTextureSRV(2, 2, XMFLOAT4(0.5f, 0.5f, 1.0f, 1));

		// Set up the materials
		std::shared_ptr<Material> matMetal = std::make_shared<Material>(
			white3, vertexShaders[0],pixelShaders[0]);
		matMetal->AddSampler("BasicSampler", sampler);
		matMetal->AddTextureSRV("Albedo", albedoSRV);
		matMetal->AddTextureSRV("NormalMap", normalSRV);
		matMetal->AddTextureSRV("RoughnessMap", roughSRV);
		matMetal->AddTextureSRV("MetalMap", metal1SRV);

		std::shared_ptr<Material> matNonMetal = std::make_shared<Material>(
			white3, vertexShaders[0], pixelShaders[0]);
		matNonMetal->AddSampler("BasicSampler", sampler);
		matNonMetal->AddTextureSRV("Albedo", albedoSRV);
		matNonMetal->AddTextureSRV("NormalMap", normalSRV);
		matNonMetal->AddTextureSRV("RoughnessMap", roughSRV);
		matNonMetal->AddTextureSRV("MetalMap", metal0SRV);

		materials.insert(materials.end(), { matMetal, matNonMetal });

		// Create the entities
		std::shared_ptr<Entity> geMetal = std::make_shared<Entity>(meshes[3], matMetal);
		std::shared_ptr<Entity> geNonMetal = std::make_shared<Entity>(meshes[3], matNonMetal);
		entities.push_back(geMetal);
		entities.push_back(geNonMetal);

		// Move and scale them
		geMetal->GetTransform()->SetPosition(i * 4.0f - 20.0f, 8, 20);
		geNonMetal->GetTransform()->SetPosition(i * 4.0f - 20.0f, 4, 20);

		geMetal->GetTransform()->SetScale(2);
		geNonMetal->GetTransform()->SetScale(2);
	}
}
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Game::CreateSolidColorTextureSRV(int width, int height, DirectX::XMFLOAT4 color)
{
	// Create an array of the color
	unsigned char* pixels = new unsigned char[width * height * 4];
	for (int i = 0; i < width * height * 4;)
	{
		pixels[i++] = (unsigned char)(color.x * 255);
		pixels[i++] = (unsigned char)(color.y * 255);
		pixels[i++] = (unsigned char)(color.z * 255);
		pixels[i++] = (unsigned char)(color.w * 255);
	}

	// Create a simple texture of the specified size
	D3D11_TEXTURE2D_DESC td = {};
	td.ArraySize = 1;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.MipLevels = 1;
	td.Height = height;
	td.Width = width;
	td.SampleDesc.Count = 1;

	// Initial data for the texture
	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = pixels;
	data.SysMemPitch = sizeof(unsigned char) * 4 * width;

	// Actually create it
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	device->CreateTexture2D(&td, &data, texture.GetAddressOf());

	// All done with pixel array
	delete[] pixels;

	// Create the shader resource view for this texture and return
	// Note: Passing in a null description creates a standard
	// SRV that has access to the entire resource (all mips, if they exist)
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	device->CreateShaderResourceView(texture.Get(), 0, srv.GetAddressOf());
	return srv;
}
void Game::CreateLights()
{
	lights.clear();

	// Right
	Light dLight1 = {};
	dLight1.type = LIGHT_TYPE_DIRECTIONAL;
	dLight1.direction = DirectX::XMFLOAT3(-1.0f, 0.0f, 0.0f);
	dLight1.color = white3;
	dLight1.intensity = 1.0f;
	// Left
	Light dLight2 = {};
	dLight2.type = LIGHT_TYPE_DIRECTIONAL;
	dLight2.direction = DirectX::XMFLOAT3(+1.0f, 0.0f, 0.0f);
	dLight2.color = white3;
	dLight2.intensity = 1.0f;
	// Back
	Light dLight3 = {};
	dLight3.type = LIGHT_TYPE_DIRECTIONAL;
	dLight3.direction = DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f);
	dLight3.color = white3;
	dLight3.intensity = 0.5f;
	// Front
	Light dLight4 = {};
	dLight4.type = LIGHT_TYPE_DIRECTIONAL;
	dLight4.direction = DirectX::XMFLOAT3(0.0f, 0.0f, +1.0f);
	dLight4.color = white3;
	dLight4.intensity = 0.5f;
	// Bottom
	Light dLight5 = {};
	dLight5.type = LIGHT_TYPE_DIRECTIONAL;
	dLight5.direction = DirectX::XMFLOAT3(0.0f, +1.0, 0.0f);
	dLight5.color = white3;
	dLight5.intensity = 1.0f;
	// Top
	Light dLight6 = {};
	dLight6.type = LIGHT_TYPE_DIRECTIONAL;
	dLight6.direction = DirectX::XMFLOAT3(0.0f, -1.0, 0.0f);
	dLight6.color = white3;
	dLight6.intensity = 0.3f;

	Light pLight1 = {};
	pLight1.type = LIGHT_TYPE_POINT;
	pLight1.direction = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f);
	pLight1.color = cyan3;
	pLight1.intensity = 1.0f;
	pLight1.position = DirectX::XMFLOAT3(4.0f, 3.0f, 0.0f);
	pLight1.range = 8.0f;

	Light pLight2 = {};
	pLight2.type = LIGHT_TYPE_POINT;
	pLight2.direction = DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f);
	pLight2.color = red3;
	pLight2.intensity = 1.0f;
	pLight2.position = DirectX::XMFLOAT3(12.0f, 2.0f, 0.0f);
	pLight2.range = 8.0f;

	lights.push_back(dLight1);
	lights.push_back(dLight2);
	lights.push_back(dLight3);
	lights.push_back(dLight4);
	lights.push_back(dLight5);
	lights.push_back(dLight6);
	lights.push_back(pLight1);
	lights.push_back(pLight2);

	lights.resize(MAX_LIGHTS);
}
// Set initial graphics API state
//  - These settings persist until we change them
//  - Some of these, like the primitive topology & input layout, probably won't change
//  - Others, like setting shaders, will need to be moved elsewhere later
void Game::InitGraphicsState()
{
	// Tell the input assembler (IA) stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our vertices?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
/// Create cameras
void Game::CreateCameras()
{
	std::shared_ptr<Camera> camera1 = std::make_shared<Camera>(
		0.0f, 5.0f, -10.0f,					// Position
		(float)(windowWidth / windowHeight),// Aspect
		5.0f,								// Move speed
		0.002f,								// Look speed
		45.0f,								// FOV
		0.001f,								// Near
		1000.0f,							// Far
		Mode::PERSPECTIVE);					// Mode

	cameras.push_back(camera1);

	activeCameraIndex = 0;
	activeCamera = cameras[activeCameraIndex];
}
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	for (auto& camera : cameras) {
		camera->UpdateProjectionMatrix((float)(windowWidth / windowHeight));
	}
}
// Update your game here - user input, move objects, AI, etc.
void Game::Update(float deltaTime, float totalTime)
{
	// - Update ImGUI - //
	IGUpdate(deltaTime);
	IGRun();

	//float rotationSpeed = 0.3f;
	//float sine = (float)sin(totalTime);
	//float cosine = (float)cos(totalTime);
	//entities[0]->GetTransform()->Rotate(0.0f, -deltaTime * rotationSpeed / 4, deltaTime * rotationSpeed);
	//entities[1]->GetTransform()->Rotate(deltaTime * rotationSpeed, 0.0f, -deltaTime * rotationSpeed/4);
	//entities[3]->GetTransform()->Rotate(-deltaTime * rotationSpeed / 4, 0.0f, deltaTime * rotationSpeed);
	//entities[4]->GetTransform()->Rotate(deltaTime * rotationSpeed, 0.0f, -deltaTime * rotationSpeed / 4);

	// - Input- //
	if (Input::GetInstance().KeyDown(VK_ESCAPE)) Quit();
	// Camera Input
	if (Input::GetInstance().KeyPress(VK_UP)) {
		activeCameraIndex = ++activeCameraIndex < cameras.size() ? activeCameraIndex : 0;
		activeCamera = cameras[activeCameraIndex];
	}
	else if (Input::GetInstance().KeyPress(VK_DOWN))
	{
		activeCameraIndex = activeCameraIndex-- > 0 ? activeCameraIndex : (int)cameras.size()-1;
		activeCamera = cameras[activeCameraIndex];
	}

	// - Update Camera - //
	activeCamera->Update(deltaTime);
}
// Clear the screen, redraw everything, present to the user
void Game::Draw(float deltaTime, float totalTime)
{
	// Clear the back buffer (erases what's on the screen)
	const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
	context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

	// Clear the depth buffer (resets per-pixel occlusion information)
	context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	for (auto& entity : entities) {
		std::shared_ptr<SimplePixelShader> ps = entity->GetMaterial()->GetPixelShader();
		ps->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		ps->SetInt("lightCount", lightCount);
		ps->SetFloat("time", totalTime); // Do I need this?
		ps->SetFloat2("resolution", DirectX::XMFLOAT2((float)this->windowWidth, (float)this->windowHeight));
		ps->SetFloat3("ambientColor", ambientColor);
		ps->SetInt("gammaCorrection", (int)gammaCorrection);
		ps->SetInt("useMetalMap", (int)useMetalMap);
		ps->SetInt("useSpecularMap", (int)useSpecularMap);
		ps->SetInt("useNormalMap", (int)useNormalMap);
		ps->SetInt("useRoughnessMap",1);
		ps->SetInt("useAlbedoTexture", (int)useAlbedoTexture);
		entity->Draw(context, activeCamera);
	}
	sky->Draw(activeCamera);

	// -- ImGUI -- //
	IGDraw();

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

// -- ImGUI -- //
void Game::IGInit()
{
	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
	ImGui::StyleColorsDark(); //StyleColorsLight() and StyleColorsClassic()
}
void Game::IGDelete()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
void Game::IGUpdate(float deltaTime)
{
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
	Input& input = Input::GetInstance();
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);
}
void Game::IGDraw()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
void Game::IGRun() {
	
	// -- Base
	//ImGui::ShowDemoWindow(); // Show the demo window
	ImGui::Begin("Inspector");
	ImGui::PushItemWidth(-160); // Negative value sets label width
	ImGui::Spacing();
	ImGui::Text("Frame rate: %.f fps", ImGui::GetIO().Framerate);
	ImGui::Text("Window Client Size: %dx%d", windowWidth, windowHeight);
	ImGui::Spacing();
	
	// Meshes
	if (ImGui::TreeNode("Meshes"))
	{
		// Loop and show the details for each mesh
		for (int i = 0; i < meshes.size(); i++)
		{
			ImGui::Text("Mesh %d: %d indices", i, meshes[i]->GetIndexCount());
		}
		// Finalize the tree node
		ImGui::TreePop();
	}

	// Entities
	if (ImGui::TreeNode("Entities"))
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
				ImGui::Spacing();
				Transform* transform = entities[i]->GetTransform();
				XMFLOAT3 pos = transform->GetPosition();
				XMFLOAT3 rot = transform->GetRotation();
				XMFLOAT3 sca = transform->GetScale();
				if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) transform->SetPosition(pos);
				if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f)) transform->SetRotation(rot);
				if (ImGui::DragFloat3("Scale", &sca.x, 0.01f)) transform->SetScale(sca);
				ImGui::Spacing();
				ImGui::Text("Mesh Index Count: %d", entities[i]->GetMesh()->GetIndexCount());

				// Only should be used if not using texture and has own material
				if (ImGui::TreeNode("Material Node", "Material", i))
				{
					ImGui::Spacing();
					std::shared_ptr<Material> mat = entities[i]->GetMaterial();
					XMFLOAT3 tint = mat->GetTint();
					float roughness = mat->GetRoughness();
					if (ImGui::ColorEdit3("Color Tint", &tint.x)) mat->SetTint(tint);
					if (ImGui::DragFloat("Roughness", &roughness, 0.01f)) mat->SetRoughness(roughness);
					ImGui::Spacing();
					ImGui::TreePop();
				}

				ImGui::Spacing();
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		// Finalize the tree node
		ImGui::TreePop();
	}

	// Cameras
	if (ImGui::TreeNode("Cameras"))
	{
		ImGui::Text("Active Camera: %d", activeCameraIndex);
		if (ImGui::Button("Prev")) {
			activeCameraIndex = activeCameraIndex-- > 0 ? activeCameraIndex : (int)cameras.size() - 1;
			activeCamera = cameras[activeCameraIndex];
		}
		ImGui::SameLine();
		if (ImGui::Button("Next")) {
			activeCameraIndex = ++activeCameraIndex < cameras.size() ? activeCameraIndex : 0;
			activeCamera = cameras[activeCameraIndex];
		}
		ImGui::Spacing();
		ImGui::PushID(0);
		if (ImGui::TreeNode("Camera", "Active Camera"))
		{
			ImGui::Spacing();
			Transform* transform = cameras[activeCameraIndex]->GetTransform();
			XMFLOAT3 pos = transform->GetPosition();
			XMFLOAT3 rot = transform->GetRotation();
			XMFLOAT3 sca = transform->GetScale();
			if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) transform->SetPosition(pos);
			if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f)) transform->SetRotation(rot);
			if (ImGui::DragFloat3("Scale", &sca.x, 0.01f)) transform->SetScale(sca);
			ImGui::Spacing();

			float nearClip = cameras[activeCameraIndex]->GetNearClip();
			float farClip = cameras[activeCameraIndex]->GetFarClip();
			if (ImGui::DragFloat("Near Clip Distance", &nearClip, 0.01f, 0.001f, 1.0f)) cameras[activeCameraIndex]->SetNearClip(nearClip);
			if (ImGui::DragFloat("Far Clip Distance", &farClip, 1.0f, 10.0f, 1000.0f)) cameras[activeCameraIndex]->SetFarClip(farClip);
			ImGui::Spacing();

			Mode mode = cameras[activeCameraIndex]->GetMode();
			int modeIndex = (int)mode;
			if (ImGui::Combo("Camera Mode", &modeIndex, "Perspective\0Orthographic"))
			{
				mode = (Mode)modeIndex;
				cameras[activeCameraIndex]->SetMode(mode);
			}
			ImGui::Spacing();

			if (mode == Mode::PERSPECTIVE)
			{
				// Init number is way off for some reason
				float fov = cameras[activeCameraIndex]->GetFieldOfView() * 180.0f / DirectX::XM_PI;
				if (ImGui::SliderFloat("Field of View (Degrees)", &fov, 0.01f, 180.0f)) cameras[activeCameraIndex]->SetFieldOfView(fov * DirectX::XM_PI / 180.0f); // Back to radians
			}
			else if (mode == Mode::ORTHOGRAPHIC)
			{
				float width = cameras[activeCameraIndex]->GetOrthographicWidth();
				if (ImGui::SliderFloat("Ortho Width", &width, 1.0f, 10.0f)) cameras[activeCameraIndex]->SetOrthographicWidth(width);
			}
			ImGui::Spacing();
			ImGui::TreePop();
		}
		ImGui::PopID();

		// Finalize the tree node
		ImGui::TreePop();
	}

	// Lights
	if (ImGui::TreeNode("Lights"))
	{
		for (int i = 0; i < lights.size(); i++)
		{
			ImGui::PushID(i);
			if (ImGui::TreeNode("Light Node", "Light %d", i))
			{
				ImGui::Spacing();
				XMFLOAT3 lColor = lights[i].color;
				XMFLOAT3 lDir = lights[i].direction;
				float lIntensity = lights[i].intensity;
				if (ImGui::DragFloat3("Color", &lColor.x, 0.01f)) lights[i].color = lColor;
				if (ImGui::DragFloat3("Direction", &lDir.x, 0.01f)) lights[i].direction = lDir;
				if (ImGui::DragFloat("Intensity", &lIntensity, 0.01f)) lights[i].intensity = lIntensity;
				ImGui::Spacing();

				int lType = lights[i].type;
				if (ImGui::Combo("Camera Mode", &lType, "Directional\0Point\0Spot"))
				{
					lights[i].type = lType;
				}
				ImGui::Spacing();

				if (lType == LIGHT_TYPE_POINT)
				{
					XMFLOAT3 lPos = lights[i].position;
					if (ImGui::DragFloat3("Position", &lPos.x, 0.01f)) lights[i].position = lPos;
					float lRange = lights[i].range;
					if (ImGui::DragFloat("Range", &lRange, 0.01f)) lights[i].range = lRange;
				}
				else if (lType == LIGHT_TYPE_SPOT)
				{
					XMFLOAT3 lPos = lights[i].position;
					if (ImGui::DragFloat3("Position", &lPos.x, 0.01f)) lights[i].position = lPos;
					float lspotFalloff = lights[i].spotFalloff;
					XMFLOAT3 lPadding = lights[i].padding;
					if (ImGui::DragFloat("Range", &lspotFalloff, 0.01f)) lights[i].spotFalloff = lspotFalloff;
				}
				ImGui::Spacing();

				ImGui::TreePop();

				//float spotFalloff;
				//float3 padding;
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	// Materials & Textures
	if (ImGui::TreeNode("Materials"))
	{
		if (ImGui::TreeNode("Global Material Controls"))
		{
			ImGui::Checkbox("Gamma Correction", &gammaCorrection);
			ImGui::Checkbox("Specular Map", &useSpecularMap);
			ImGui::Checkbox("Albedo Texture", &useAlbedoTexture);
			ImGui::Checkbox("Normal Map", &useNormalMap);
			ImGui::Checkbox("Roughness Map", &useRoughnessMap);
			ImGui::Checkbox("Metalness Map", &useMetalMap);

			ImGui::TreePop();
			ImGui::Spacing();
		}

		for (int i = 0; i < materials.size(); i++)
		{
			ImGui::PushID(i);
			if (ImGui::TreeNode("Material Node", "Material %d", i))
			{
				ImGui::Spacing();

				XMFLOAT3 mTint = materials[i]->GetTint();
				float mRoughness = materials[i]->GetRoughness();
				XMFLOAT2 mUVOffset = materials[i]->GetUVOffset();
				XMFLOAT2 mUVScale = materials[i]->GetUVScale();
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV = materials[i]->GetTextureSRV("SurfaceTexture");
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SpecularSRV = materials[i]->GetTextureSRV("SpecularMap");

				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedoSRV = materials[i]->GetTextureSRV("Albedo");
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMapSRV = materials[i]->GetTextureSRV("NormalMap");
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughnessMapSRV = materials[i]->GetTextureSRV("RoughnessMap");
				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMapSRV = materials[i]->GetTextureSRV("MetalMap");
				if (ImGui::ColorEdit3("Color Tint", &mTint.x)) materials[i]->SetTint(mTint);
				if (ImGui::DragFloat("Roughness", &mRoughness, 0.01f)) materials[i]->SetRoughness(mRoughness);
				if (ImGui::DragFloat2("UV Offset", &mUVOffset.x, 0.01f)) materials[i]->SetUVOffset(mUVOffset);
				if (ImGui::DragFloat2("UV Scale", &mUVScale.x, 0.01f)) materials[i]->SetUVOffset(mUVScale);
				ImGui::Spacing();

				if (ImGui::TreeNode("Textures"))
				{
					ImGui::Image(textureSRV.Get(), ImVec2(100.0f, 100.0f));
					ImGui::SameLine();
					ImGui::Image(SpecularSRV.Get(), ImVec2(100.0f, 100.0f)); 
					ImGui::NewLine();
					ImGui::Image(albedoSRV.Get(), ImVec2(100.0f, 100.0f));
					ImGui::SameLine();
					ImGui::Image(normalMapSRV.Get(), ImVec2(100.0f, 100.0f));
					ImGui::NewLine();
					ImGui::Image(roughnessMapSRV.Get(), ImVec2(100.0f, 100.0f));
					ImGui::SameLine();
					ImGui::Image(metalMapSRV.Get(), ImVec2(100.0f, 100.0f));
					ImGui::Spacing();
					ImGui::TreePop();
				}


				ImGui::Spacing();
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	ImGui::End();
}

// -- Geometry -- //
void Game::CreateGeometry()
{
	// Create meshes
	meshes.push_back(Create2DEquilateralTriangle(device, XMFLOAT3(0.5f, 0.0f, 0.0f), .5f, red4));
	meshes.push_back(Create2DRightTriangle(device, XMFLOAT3(), .5f, green4));
	meshes.push_back(Create2DSquare(device, XMFLOAT3(-0.7f, 0.0f, 0.0f), .2f, grey4));
	meshes.push_back(Create2DRectangle(device, XMFLOAT3(-0.1f, 0.5f, 0.0f), .3f, .1f, blue4));
	meshes.push_back(CreateDefault2DSquare(device, black4));
	meshes.push_back(Create2DSquare(device, XMFLOAT3(), .2f, black4));
}
//	Issue: size is not independent of computor hight and width 
std::shared_ptr<Mesh> Game::Create2DEquilateralTriangle(Microsoft::WRL::ComPtr<ID3D11Device> device, XMFLOAT3 position, float scale, XMFLOAT4 color) {
	// Equilateral Triangle
	float maxX, maxY;
	maxX = maxY = scale / 2;

	Vertex vertices[] =
	{
		{ XMFLOAT3(position.x, position.y + maxY, +0.0f), DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top left
		{ XMFLOAT3(position.x + maxY, position.y - maxY, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Right
		{ XMFLOAT3(position.x - maxY, position.y - maxY, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Left
	};
	unsigned int indices[] = { 0, 1, 2 }; // Clockwise
	return std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices), device);
};
std::shared_ptr<Mesh> Game::Create2DRightTriangle(Microsoft::WRL::ComPtr<ID3D11Device> device, XMFLOAT3 position, float scale, XMFLOAT4 color) {
	// Equilateral Triangle
	float maxX, maxY;
	maxX = maxY = scale / 2;

	Vertex vertices[] =
	{
		{ XMFLOAT3(position.x - maxY, position.y + maxY, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top left
		{ XMFLOAT3(position.x + maxY, position.y - maxY, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Right
		{ XMFLOAT3(position.x - maxY, position.y - maxY, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Left
	};
	unsigned int indices[] = { 0, 1, 2 }; // Clockwise
	return std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices), device);
};
std::shared_ptr<Mesh> Game::Create2DSquare(Microsoft::WRL::ComPtr<ID3D11Device> device, XMFLOAT3 position, float size, XMFLOAT4 color) {
	float halfSize = size / 2;

	Vertex vertices[] =
	{
		{ XMFLOAT3(position.x - halfSize, position.y + halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top left
		{ XMFLOAT3(position.x + halfSize, position.y + halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top Right
		{ XMFLOAT3(position.x + halfSize, position.y - halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Right
		{ XMFLOAT3(position.x - halfSize, position.y - halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Left
	};
	unsigned int indices[] = 
	{ 
		0, 1, 2,
		0, 2, 3
	}; // Clockwise
	return std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices), device);
};
std::shared_ptr<Mesh> Game::CreateDefault2DSquare(Microsoft::WRL::ComPtr<ID3D11Device> device, XMFLOAT4 color) {
	XMFLOAT3 position(0.0f,0.0f,0.0f);
	float halfSize = 0.5f;

	Vertex vertices[] =
	{
		{ XMFLOAT3(position.x - halfSize, position.y + halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top left
		{ XMFLOAT3(position.x + halfSize, position.y + halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top Right
		{ XMFLOAT3(position.x + halfSize, position.y - halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Right
		{ XMFLOAT3(position.x - halfSize, position.y - halfSize, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Left
	};
	unsigned int indices[] =
	{
		0, 1, 2,
		0, 2, 3
	}; // Clockwise
	return std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices), device);
};
std::shared_ptr<Mesh> Game::Create2DRectangle(Microsoft::WRL::ComPtr<ID3D11Device> device, XMFLOAT3 position, float width, float height, XMFLOAT4 color) {
	// Equilateral Triangle
	float halfWidth = width / 2;
	float halfHeight = height / 2;

	Vertex vertices[] =
	{
		{ XMFLOAT3(position.x - halfWidth, position.y + halfHeight, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top left
		{ XMFLOAT3(position.x + halfWidth, position.y + halfHeight, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Top Right
		{ XMFLOAT3(position.x + halfWidth, position.y - halfHeight, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Right
		{ XMFLOAT3(position.x - halfWidth, position.y - halfHeight, +0.0f),DirectX::XMFLOAT3(0, 0, -1), DirectX::XMFLOAT2(0,0) },	// Bottom Left
	};
	unsigned int indices[] =
	{
		0, 1, 2,
		0, 2, 3
	}; // Clockwise
	return std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices), device);
};