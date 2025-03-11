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
	// Variables
	offset = XMFLOAT3(0.25f, 0.0f, 0.0f);
	tint = XMFLOAT4(1.0f, 1.2f, 1.0f, 1.0f);

	// -- ImGUI -- //
	IGInit();

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateGeometry();
	
	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
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

	unsigned int cbSize = sizeof(VertexShaderExternalData);
	cbSize = ((cbSize +15)/16)*16; // Truncate
	// Describe the constant buffer
	D3D11_BUFFER_DESC cbDesc = {}; // Sets struct to all zeros
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = cbSize; // Must be a multiple of 16
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	device->CreateBuffer(&cbDesc, 0, vsConstantBuffer.GetAddressOf());
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

// --------------------------------------------------------
// Creates the geometry we're going to draw
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// -- Mesh 1 -- //
	Vertex vertices1[] =
	{
		{ XMFLOAT3(-0.25f, +0.25f, +0.0f), black },	// Top left
		{ XMFLOAT3(+0.25f, -0.25f, +0.0f), green },	// Bottom Right
		{ XMFLOAT3(-1.00f, -1.00f, +0.0f), blue },	// Bottom Left
	};
	unsigned int indices1[] = { 0, 1, 2 }; // Clockwise
	std::shared_ptr<Mesh> mesh1 = std::make_shared<Mesh>(vertices1, ARRAYSIZE(vertices1), indices1, ARRAYSIZE(indices1), device);
	meshes.push_back(mesh1);

	// -- Mesh 2 -- //
	float top = +0.70f;
	float tom = +0.10f;
	float left = -1.00f;
	float right = +-0.50f;
	Vertex vertices2[] =
	{
		{ XMFLOAT3(left, top, 0.0f), red },	// Top left
		{ XMFLOAT3(right, top, 0.0f), grey },	// Top right
		{ XMFLOAT3(right, tom, 0.0f), red },	// Bottom right
		{ XMFLOAT3(left, tom, 0.0f), black }	// Bottom left
	};
	unsigned int indices2[] =
	{
		0, 1, 2,	
		0, 2, 3		
	}; // CW rotation
	std::shared_ptr<Mesh> mesh2 = std::make_shared<Mesh>(vertices2, ARRAYSIZE(vertices2), indices2, ARRAYSIZE(indices2), device);
	meshes.push_back(mesh2);

	// -- Mesh 3 -- //
	top = +0.50f;
	tom = -0.50f;
	left = +0.30f;
	right = +0.90f;
	float middleV = (top + tom) / 2;
	float shiftMiddle = -0.2f;
	Vertex vertices3[] =
	{
		{ XMFLOAT3(left,top, 0.0f), black },
		{ XMFLOAT3(right,top, 0.0f), blue },
		{ XMFLOAT3(right + shiftMiddle,middleV, 0.0f), black },
		{ XMFLOAT3(left + shiftMiddle,middleV, 0.0f), blue },
		{ XMFLOAT3(right,tom, 0.0f), blue },
		{ XMFLOAT3(left,tom, 0.0f), black }
	};
	unsigned int indices3[] =
	{
		0, 1, 2,
		0, 2, 3,
		3, 2, 4,
		3, 4, 5
	};
	std::shared_ptr<Mesh> mesh3 = std::make_shared<Mesh>(vertices3, ARRAYSIZE(vertices3), indices3, ARRAYSIZE(indices3), device);
	meshes.push_back(mesh3);
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
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// -- ImGUI -- //
	IGUpdate(deltaTime);
	IGRun();

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();
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
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	VertexShaderExternalData vsData;
	vsData.colorTint = tint;
	vsData.offset = offset;
	D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
	context->Map(vsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer); // lock from GPU
	memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));
	context->Unmap(vsConstantBuffer.Get(), 0); // unlock from GPU
	context->VSSetConstantBuffers(
		0, // Which slot (register) to bind the buffer to?
		1, // How many are we activating? Can do multiple at once
		vsConstantBuffer.GetAddressOf()); // Array of buffers (or the address of one)

	// -- Draw Meshes -- //
	for (auto& mesh : meshes)
	{
		//ChangeDataInConstantBuffer() // Map, memcpy, unmap if I wanted to have for each mesh
		mesh->SetBuffersAndDraw(context);
	}

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
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

// ImGUI
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
	
	ImGui::ShowDemoWindow(); // Show the demo window

	// Examples
	{
		//// -- Text -- //
		//ImGui::Text("This is the");
		//int value = 0;
		//ImGui::SliderInt("Choose a number", &value, 0, 100);

		//// -- Sliders & Vectors -- //
		//XMFLOAT3 vec(10.0f, -2.0f, 99.0f);
		//// Provide the address of the first element to create a
		//// 3-component, draggable editor for a vector
		//ImGui::DragFloat3("Edit a vector", &vec.x);
		//XMFLOAT4 color(1.0f, 0.0f, 0.5f, 1.0f);
		//// You can create a 3 or 4-component color editor, too!
		//// Notice the two different function names below (ending with 3 and 4)
		//ImGui::ColorEdit3("3 - component(RGB) color editor", &color.x);
		//ImGui::ColorEdit4("4 - component(RGBA) color editor", &color.x);

		// -- Naming -- //
		// Each float3 editor below wants the same name (“Direction”). To prevent conflicts
		// conflicts, a double pound sign “##” and unique characters are added.
		// The “##” and anything after are not displayed in the UI.
		//ImGui::Text("Light 1");
		//ImGui::DragFloat3("Direction##1", &light1.x);
		//ImGui::Text("Light 2");
		//ImGui::DragFloat3("Direction##2", &light2.x);

		//// -- Create Window -- //
		//ImGui::Begin("My First Window"); // Everything after is part of the window
		//ImGui::Text("This text is in the window");
		//// value is an integer variable
		//// Create a slider from 0-100 which reads and updates value
		//ImGui::SliderInt("Choose a number", &value, 0, 100);
		//// Create a button and test for a click
		//if (ImGui::Button("Press to increment"))
		//{
		//	value++; // Adds to value when clicked
		//}
		//ImGui::End(); // Ends the current window
	}
	ImGui::Begin("Custom Interface");
	// Set a specific amount of space for widget labels
	ImGui::PushItemWidth(-160); // Negative value sets label width
	//if (ImGui::TreeNode("App Details"))
	//{
	//	ImGui::Spacing();
	//	ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
	//	ImGui::Text("Window Client Size: %dx%d", windowWidth, windowHeight);
	//
	//	// Should we show the demo window?
	//	if (ImGui::Button(showUIDemoWindow ? "Hide ImGui Demo Window" : "Show ImGui Demo Window"))
	//		showUIDemoWindow = !showUIDemoWindow;
	//
	//	ImGui::Spacing();
	//
	//	// Finalize the tree node
	//	ImGui::TreePop();
	//}

	ImGui::Text("Frame Rate: %.f", ImGui::GetIO().Framerate);
	XMFLOAT2 windowSize(windowWidth, windowHeight);
	ImGui::DragFloat2("Window Size", &windowSize.x); // needs address of first element // I dont want it to drag
	ImGui::DragFloat3("Offset", &offset.x); // needs address of first element // I dont want it to drag
	ImGui::ColorEdit4("Tint", &tint.x); // needs address of first element // I dont want it to drag
	
	// === Meshes ===
	//if (ImGui::TreeNode("Meshes"))
	//{
	//	// Loop and show the details for each mesh
	//	for (int i = 0; i < meshes.size(); i++)
	//	{
	//		ImGui::Text("Mesh %d: %d indices", i, meshes[i]->GetIndexCount());
	//	}
	//	// Finalize the tree node
	//	ImGui::TreePop();
	//}
	ImGui::End(); // Ends the current window
}