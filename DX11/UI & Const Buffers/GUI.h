#pragma once
#include "DXCore.h"
#include <wrl/client.h> // for ComPtr
#include "Input.h"

// ImGUI
#include "ImGUI/imgui.h"
#include "ImGUI/Windows/imgui_impl_win32.h"
#include "ImGUI/DX11/imgui_impl_dx11.h"

class GUI
{
public:
	GUI();
	GUI(HWND hWnd, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	~GUI();
	void Update(float deltaTime, float windowWidth, float windowHeight);
	void Draw();
	//void DeclareMessageHandler
};

