#include "Sky.h"
#include <WICTextureLoader.h>

// Author: Chris Cascioli
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::CreateCubemap(
	const wchar_t* right, const wchar_t* left, 
	const wchar_t* up, const wchar_t* down, 
	const wchar_t* front, const wchar_t* back)
{
	// Load the 6 textures into an array.
	// - We need references to the TEXTURES, not SHADER RESOURCE VIEWS!
	// - Explicitly NOT generating mipmaps, as we don't need them for the sky!
	// - Order matters here!  +X, -X, +Y, -Y, +Z, -Z
	Microsoft::WRL::ComPtr<ID3D11Texture2D> textures[6] = {};
	DirectX::CreateWICTextureFromFile(device.Get(), right, (ID3D11Resource**)textures[0].GetAddressOf(), 0);
	DirectX::CreateWICTextureFromFile(device.Get(), left, (ID3D11Resource**)textures[1].GetAddressOf(), 0);
	DirectX::CreateWICTextureFromFile(device.Get(), up, (ID3D11Resource**)textures[2].GetAddressOf(), 0);
	DirectX::CreateWICTextureFromFile(device.Get(), down, (ID3D11Resource**)textures[3].GetAddressOf(), 0);
	DirectX::CreateWICTextureFromFile(device.Get(), front, (ID3D11Resource**)textures[4].GetAddressOf(), 0);
	DirectX::CreateWICTextureFromFile(device.Get(), back, (ID3D11Resource**)textures[5].GetAddressOf(), 0);

	// We'll assume all of the textures are the same color format and resolution,
	// so get the description of the first texture
	D3D11_TEXTURE2D_DESC faceDesc = {};
	textures[0]->GetDesc(&faceDesc);

	// Describe the resource for the cube map, which is simply 
	// a "texture 2d array" with the TEXTURECUBE flag set.  
	// This is a special GPU resource format, NOT just a 
	// C++ array of textures!!!
	D3D11_TEXTURE2D_DESC cubeDesc = {};
	cubeDesc.ArraySize = 6;            // Cube map!
	cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // We'll be using as a texture in a shader
	cubeDesc.CPUAccessFlags = 0;       // No read back
	cubeDesc.Format = faceDesc.Format; // Match the loaded texture's color format
	cubeDesc.Width = faceDesc.Width;   // Match the size
	cubeDesc.Height = faceDesc.Height; // Match the size
	cubeDesc.MipLevels = 1;            // Only need 1
	cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // This should be treated as a CUBE, not 6 separate textures
	cubeDesc.Usage = D3D11_USAGE_DEFAULT; // Standard usage
	cubeDesc.SampleDesc.Count = 1;
	cubeDesc.SampleDesc.Quality = 0;

	// Create the final texture resource to hold the cube map
	Microsoft::WRL::ComPtr<ID3D11Texture2D> cubeMapTexture;
	device->CreateTexture2D(&cubeDesc, 0, cubeMapTexture.GetAddressOf());

	// Loop through the individual face textures and copy them,
	// one at a time, to the cube map texure
	for (int i = 0; i < 6; i++)
	{
		// Calculate the subresource position to copy into
		unsigned int subresource = D3D11CalcSubresource(
			0,  // Which mip (zero, since there's only one)
			i,  // Which array element?
			1); // How many mip levels are in the texture?

		// Copy from one resource (texture) to another
		context->CopySubresourceRegion(
			cubeMapTexture.Get(),  // Destination resource
			subresource,           // Dest subresource index (one of the array elements)
			0, 0, 0,               // XYZ location of copy
			textures[i].Get(),     // Source resource
			0,                     // Source subresource index (we're assuming there's only one)
			0);                    // Source subresource "box" of data to copy (zero means the whole thing)
	}

	// At this point, all of the faces have been copied into the 
	// cube map texture, so we can describe a shader resource view for it
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = cubeDesc.Format;         // Same format as texture
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
	srvDesc.TextureCube.MipLevels = 1;        // Only need access to 1 mip
	srvDesc.TextureCube.MostDetailedMip = 0;  // Index of the first mip we want to see

	// Make the SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
	device->CreateShaderResourceView(cubeMapTexture.Get(), &srvDesc, cubeSRV.GetAddressOf());

	// Send back the SRV, which is what we need for our shaders
	return cubeSRV;
}
void Sky::Init()
{
	// Renders texture inside shape
	D3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_FRONT; // Draw inside
	rastDesc.FillMode = D3D11_FILL_SOLID;
	rastDesc.DepthClipEnable = true;
	device->CreateRasterizerState(&rastDesc, rasterizerOptions.GetAddressOf());

	// Allows pixels with depth of 1.0 which our skybox is (z/w = 1)
	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = true;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	device->CreateDepthStencilState(&depthDesc, depthState.GetAddressOf());
}

Sky::Sky(
	std::shared_ptr<Mesh> mesh,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 
	Microsoft::WRL::ComPtr<ID3D11Device> device, 
	const wchar_t* right, const wchar_t* left, 
	const wchar_t* up, const wchar_t* down, 
	const wchar_t* front, const wchar_t* back) :
	mesh(mesh),
	device(device),
	samplerOptions(samplerOptions)
{
	Init();
	cubeTextureSRV = CreateCubemap(right, left, up, down, front, back);
	//CreateDDSTextureFromFile(device.Get(), cubemapDDSFile, 0, skySRV.GetAddressOf()); // could have a constructor with this and no char params
}

Sky::Sky(
	std::shared_ptr<Mesh> mesh,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	std::shared_ptr<SimpleVertexShader>	vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader,
	const wchar_t* right, const wchar_t* left,
	const wchar_t* up, const wchar_t* down,
	const wchar_t* front, const wchar_t* back) :
	mesh(mesh),
	device(device),
	context(context),
	samplerOptions(samplerOptions),
	vertexShader(vertexShader),
	pixelShader(pixelShader)
{
	Init();
	cubeTextureSRV = CreateCubemap(right, left, up, down, front, back);
}

void Sky::Draw(
	std::shared_ptr <Camera> camera)
{
	context->RSSetState(rasterizerOptions.Get());
	context->OMSetDepthStencilState(depthState.Get(),0);

	vertexShader->SetShader();
	pixelShader->SetShader();

	vertexShader->SetMatrix4x4("viewMatrix", camera->GetView());
	vertexShader->SetMatrix4x4("projectionMatrix", camera->GetProjection());
	vertexShader->CopyAllBufferData();

	pixelShader->SetShaderResourceView("Cube", cubeTextureSRV);
	pixelShader->SetSamplerState("Sampler", samplerOptions);

	mesh->SetBuffersAndDraw(context);

	context->RSSetState(0);
	context->OMSetDepthStencilState(0, 0);
}

void Sky::Draw(
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	std::shared_ptr <Camera> camera,
	std::shared_ptr<SimpleVertexShader>	vertexShader,
	std::shared_ptr<SimplePixelShader> pixelShader)
{
	context->RSSetState(rasterizerOptions.Get());
	context->OMSetDepthStencilState(depthState.Get(), 0);

	vertexShader->SetShader();
	pixelShader->SetShader();

	vertexShader->SetMatrix4x4("viewMatrix", camera->GetView());
	vertexShader->SetMatrix4x4("projectionMatrix", camera->GetProjection());
	pixelShader->CopyAllBufferData();

	pixelShader->SetShaderResourceView("Cube", cubeTextureSRV);
	pixelShader->SetSamplerState("Sampler", samplerOptions);

	mesh->SetBuffersAndDraw(context);

	context->RSSetState(0);
	context->OMSetDepthStencilState(0, 0);
}
