#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"
#include "BufferStructs.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

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
		true)				// Show extra stats (fps) in title bar?
{
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
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	IGDelete();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	IGInit();				// Init Imgui
	LoadShaders();			// Init Shaders
	CreateGeometry();		// Init geometry
	InitGraphicsState();	// Init Graphics
	CreateConstantBuffer(); // Init Constant Buffer
	CreateCameras();		// Init Cameras
}
// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	// BLOBs (or Binary Large OBjects) for reading raw data from external files
	// - This is a simplified way of handling big chunks of external data
	// - Literally just a big array of bytes read from a file
	ID3DBlob* pixelShaderBlob;
	ID3DBlob* vertexShaderBlob;

	// Loading shaders
	//  - Visual Studio will compile our shaders at build time
	//  - They are saved as .cso (Compiled Shader Object) files
	//  - We need to load them when the application starts
	{
		// Read our compiled shader code files into blobs
		// - Essentially just "open the file and plop its contents here"
		// - Uses the custom FixPath() helper from Helpers.h to ensure relative paths
		// - Note the "L" before the string - this tells the compiler the string uses wide characters
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);

		// Create the actual Direct3D shaders on the GPU
		device->CreatePixelShader(
			pixelShaderBlob->GetBufferPointer(),	// Pointer to blob's contents
			pixelShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			pixelShader.GetAddressOf());			// Address of the ID3D11PixelShader pointer

		device->CreateVertexShader(
			vertexShaderBlob->GetBufferPointer(),	// Get a pointer to the blob's contents
			vertexShaderBlob->GetBufferSize(),		// How big is that data?
			0,										// No classes in this shader
			vertexShader.GetAddressOf());			// The address of the ID3D11VertexShader pointer
	}

	// Create an input layout 
	//  - This describes the layout of data sent to a vertex shader
	//  - In other words, it describes how to interpret data (numbers) in a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the vertex shader blob above)
	{
		D3D11_INPUT_ELEMENT_DESC inputElements[2] = {};

		// Set up the first element - a position, which is 3 float values
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;				// Most formats are described as color channels; really it just means "Three 32-bit floats"
		inputElements[0].SemanticName = "POSITION";							// This is "POSITION" - needs to match the semantics in our vertex shader input!
		inputElements[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// How far into the vertex is this?  Assume it's after the previous element

		// Set up the second element - a color, which is 4 more float values
		inputElements[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;			// 4x 32-bit floats
		inputElements[1].SemanticName = "COLOR";							// Match our vertex shader input!
		inputElements[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;	// After the previous element

		// Create the input layout, verifying our description against actual shader code
		device->CreateInputLayout(
			inputElements,							// An array of descriptions
			2,										// How many elements in that array?
			vertexShaderBlob->GetBufferPointer(),	// Pointer to the code of a shader that uses this layout
			vertexShaderBlob->GetBufferSize(),		// Size of the shader code that uses this layout
			inputLayout.GetAddressOf());			// Address of the resulting ID3D11InputLayout pointer
	}
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
	
	// Ensure the pipeline knows how to interpret all the numbers stored in
	// the vertex buffer. For this course, all of your vertices will probably
	// have the same layout, so we can just set this once at startup.
	context->IASetInputLayout(inputLayout.Get());
	
	// Set the active vertex and pixel shaders
	//  - Once you start applying different shaders to different objects,
	//    these calls will need to happen multiple times per frame
	context->VSSetShader(vertexShader.Get(), 0, 0);
	context->PSSetShader(pixelShader.Get(), 0, 0);
}
// Creates Const buffer
void Game::CreateConstantBuffer() {
	unsigned int cbSize = sizeof(VertexShaderExternalData);
	cbSize = ((cbSize + 15) / 16) * 16; // Truncate
	// Describe & create the constant buffer
	D3D11_BUFFER_DESC cbDesc = {}; // Sets struct to all zeros
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = cbSize; // Must be a multiple of 16
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	device->CreateBuffer(&cbDesc, 0, vsConstantBuffer.GetAddressOf()); // Doesn't cbDesc delete itself due to scope?
}
// Create cameras
void Game::CreateCameras()
{
	std::shared_ptr<Camera> camera1 = std::make_shared<Camera>(
		-2.0f, 2.0f, -5.0f,					// Position
		(float)(windowWidth / windowHeight),// Aspect
		5.0f,								// Move speed
		0.002f,								// Look speed
		45.0f,								// FOV
		0.001f,								// Near
		1000.0f,							// Far
		Mode::PERSPECTIVE);					// Mode

	std::shared_ptr<Camera> camera2 = std::make_shared<Camera>(
		0.0f, 0.0f, -5.0f,
		(float)(windowWidth / windowHeight),
		3.0f,
		0.001f,
		50.0f,
		0.001f,
		1000.0f,
		Mode::ORTHOGRAPHIC);

	std::shared_ptr<Camera> camera3 = std::make_shared<Camera>(
		2.0f, -2.0f, -5.0f,
		(float)(windowWidth / windowHeight),
		3.0f,
		0.002f,
		60.0f,
		0.01f,
		100.0f,
		Mode::PERSPECTIVE);

	cameras.push_back(camera1);
	cameras.push_back(camera2);
	cameras.push_back(camera3);

	activeCameraIndex = 0;
	activeCamera = cameras[activeCameraIndex];
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
	for (auto& camera : cameras) {
		camera->UpdateProjectionMatrix((float)(windowWidth / windowHeight));
	}
}
// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// - Update ImGUI - //
	IGUpdate(deltaTime);
	IGRun();

	// - Input- //
	if (Input::GetInstance().KeyDown(VK_ESCAPE)) Quit();
	// Camera Input
	if (Input::GetInstance().KeyPress(VK_UP)) {
		activeCameraIndex = ++activeCameraIndex < cameras.size() ? activeCameraIndex : 0;
		activeCamera = cameras[activeCameraIndex];
	}
	else if (Input::GetInstance().KeyPress(VK_DOWN))
	{
		activeCameraIndex = activeCameraIndex-- > 0 ? activeCameraIndex : cameras.size()-1;
		activeCamera = cameras[activeCameraIndex];
	}

	// - Update Geometry - //
	float rotationSpeed = 0.3f;
	float sine = (float)sin(totalTime);
	float cosine = (float)cos(totalTime);
	entities[0]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * rotationSpeed);
	entities[1]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * rotationSpeed);
	entities[2]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * rotationSpeed);
	entities[3]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * rotationSpeed);
	entities[4]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * rotationSpeed);
	entities[5]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * rotationSpeed);
	entities[6]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * -rotationSpeed);
	entities[7]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * rotationSpeed);

	float scale = sine * 0.4f + 0.6f; // 0.5 - 1.5
	entities[3]->GetTransform()->SetScale(scale, scale, scale);
	entities[2]->GetTransform()->SetPosition(sine, 0, 0);

	// - Update Camera - //
	activeCamera->Update(deltaTime);
}
// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	for (auto& entity : entities) {
		entity->Draw(context, vsConstantBuffer, activeCamera);
	}

	// Frame END
	{
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
	
	// -- Meshes
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

	// -- Entities
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
			activeCameraIndex = activeCameraIndex-- > 0 ? activeCameraIndex : cameras.size() - 1;
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
		// Loop and show the details for each entity
		//for (int i = 0; i < cameras.size(); i++)
		//{
		//	ImGui::PushID(i);
		//	if (ImGui::TreeNode("Camera", "Camera %d", i))
		//	{
		//		ImGui::Spacing();
		//
		//		Transform* transform = cameras[i]->GetTransform();
		//		XMFLOAT3 pos = transform->GetPosition();
		//		XMFLOAT3 rot = transform->GetRotation();
		//		XMFLOAT3 sca = transform->GetScale();
		//		if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) transform->SetPosition(pos);
		//		if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f)) transform->SetRotation(rot);
		//		if (ImGui::DragFloat3("Scale", &sca.x, 0.01f)) transform->SetScale(sca);
		//		ImGui::Spacing();
		//
		//		float nearClip = cameras[i]->GetNearClip();
		//		float farClip = cameras[i]->GetFarClip();
		//		if (ImGui::DragFloat("Near Clip Distance", &nearClip, 0.01f, 0.001f, 1.0f)) cameras[i]->SetNearClip(nearClip);
		//		if (ImGui::DragFloat("Far Clip Distance", &farClip, 1.0f, 10.0f, 1000.0f)) cameras[i]->SetFarClip(farClip);
		//		ImGui::Spacing();
		//
		//		Mode mode = cameras[i]->GetMode();
		//		int modeIndex = (int)mode;
		//		if (ImGui::Combo("Camera Mode", &modeIndex, "Perspective\0Orthographic"))
		//		{
		//			mode = (Mode)modeIndex;
		//			cameras[i]->SetMode(mode);
		//		}
		//		ImGui::Spacing();
		//
		//		if (mode == Mode::PERSPECTIVE)
		//		{
		//			// Init number is way off for some reason
		//			float fov = cameras[i]->GetFieldOfView() * 180.0f / DirectX::XM_PI;
		//			if (ImGui::SliderFloat("Field of View (Degrees)", &fov, 0.01f, 180.0f)) cameras[i]->SetFieldOfView(fov * DirectX::XM_PI / 180.0f); // Back to radians
		//		}
		//		else if (mode == Mode::ORTHOGRAPHIC)
		//		{
		//			float wid = cameras[i]->GetOrthographicWidth();
		//			if (ImGui::SliderFloat("Ortho Width", &wid, 1.0f, 10.0f)) cameras[i]->SetOrthographicWidth(wid);
		//		}
		//		ImGui::Spacing();
		//		ImGui::TreePop();
		//	}
		//	ImGui::PopID();
		//}

		// Finalize the tree node
		ImGui::TreePop();
	}

	ImGui::End(); // Ends the current window
}

// -- Geometry -- //
void Game::CreateGeometry()
{
	// Create meshes
	meshes.push_back(Create2DEquilateralTriangle(device, XMFLOAT3(0.5f, 0.0f, 0.0f), .5f, red));
	meshes.push_back(Create2DRightTriangle(device, XMFLOAT3(), .5f, green));
	meshes.push_back(Create2DSquare(device, XMFLOAT3(-0.7f, 0.0f, 0.0f), .2f, grey));
	meshes.push_back(Create2DRectangle(device, XMFLOAT3(-0.1f, 0.5f, 0.0f), .3f, .1f, blue));
	meshes.push_back(CreateDefault2DSquare(device, black));
	meshes.push_back(Create2DSquare(device, XMFLOAT3(), .2f, black));


	// Create the game entities
	std::shared_ptr<Entity> e1 = std::make_shared<Entity>(meshes[0]);
	std::shared_ptr<Entity> e2 = std::make_shared<Entity>(meshes[1]);
	std::shared_ptr<Entity> e3 = std::make_shared<Entity>(meshes[2]);
	std::shared_ptr<Entity> e4 = std::make_shared<Entity>(meshes[2]);
	std::shared_ptr<Entity> e5 = std::make_shared<Entity>(meshes[3]);
	std::shared_ptr<Entity> e6 = std::make_shared<Entity>(meshes[4]);
	std::shared_ptr<Entity> e7 = std::make_shared<Entity>(meshes[4]);
	std::shared_ptr<Entity> e8 = std::make_shared<Entity>(meshes[5]);

	// Add entities
	entities.push_back(e1);
	entities.push_back(e2);
	entities.push_back(e3);
	entities.push_back(e4);
	entities.push_back(e5);
	entities.push_back(e6);
	entities.push_back(e7);
	entities.push_back(e8);

	entities[7]->GetTransform()->SetPosition(0.7f, 0.7f, 0.0f);
}
//	Issue: size is not independent of computor hight and width 
std::shared_ptr<Mesh> Game::Create2DEquilateralTriangle(Microsoft::WRL::ComPtr<ID3D11Device> device, XMFLOAT3 position, float scale, XMFLOAT4 color) {
	// Equilateral Triangle
	float maxX, maxY;
	maxX = maxY = scale / 2;

	Vertex vertices[] =
	{
		{ XMFLOAT3(position.x, position.y + maxY, +0.0f), color },	// Top left
		{ XMFLOAT3(position.x + maxY, position.y - maxY, +0.0f), color },	// Bottom Right
		{ XMFLOAT3(position.x - maxY, position.y - maxY, +0.0f), color },	// Bottom Left
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
		{ XMFLOAT3(position.x - maxY, position.y + maxY, +0.0f), color },	// Top left
		{ XMFLOAT3(position.x + maxY, position.y - maxY, +0.0f), color },	// Bottom Right
		{ XMFLOAT3(position.x - maxY, position.y - maxY, +0.0f), color },	// Bottom Left
	};
	unsigned int indices[] = { 0, 1, 2 }; // Clockwise
	return std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices), device);
};
std::shared_ptr<Mesh> Game::Create2DSquare(Microsoft::WRL::ComPtr<ID3D11Device> device, XMFLOAT3 position, float size, XMFLOAT4 color) {
	float halfSize = size / 2;

	Vertex vertices[] =
	{
		{ XMFLOAT3(position.x - halfSize, position.y + halfSize, +0.0f), color },	// Top left
		{ XMFLOAT3(position.x + halfSize, position.y + halfSize, +0.0f), color },	// Top Right
		{ XMFLOAT3(position.x + halfSize, position.y - halfSize, +0.0f), color },	// Bottom Right
		{ XMFLOAT3(position.x - halfSize, position.y - halfSize, +0.0f), color },	// Bottom Left
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
		{ XMFLOAT3(position.x - halfSize, position.y + halfSize, +0.0f), color },	// Top left
		{ XMFLOAT3(position.x + halfSize, position.y + halfSize, +0.0f), color },	// Top Right
		{ XMFLOAT3(position.x + halfSize, position.y - halfSize, +0.0f), color },	// Bottom Right
		{ XMFLOAT3(position.x - halfSize, position.y - halfSize, +0.0f), color },	// Bottom Left
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
		{ XMFLOAT3(position.x - halfWidth, position.y + halfHeight, +0.0f), color },	// Top left
		{ XMFLOAT3(position.x + halfWidth, position.y + halfHeight, +0.0f), color },	// Top Right
		{ XMFLOAT3(position.x + halfWidth, position.y - halfHeight, +0.0f), color },	// Bottom Right
		{ XMFLOAT3(position.x - halfWidth, position.y - halfHeight, +0.0f), color },	// Bottom Left
	};
	unsigned int indices[] =
	{
		0, 1, 2,
		0, 2, 3
	}; // Clockwise
	return std::make_shared<Mesh>(vertices, ARRAYSIZE(vertices), indices, ARRAYSIZE(indices), device);
};