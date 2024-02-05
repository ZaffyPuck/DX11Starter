#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class DX12Utility
{
public:
	// Gets the one and only instance of this class
	static DX12Utility& GetInstance()
	{
		if (!instance)
		{
			instance = new DX12Utility();
		}

		return *instance;
	}

	~DX12Utility();

	// Initialization for singleton
	void Initialize(
		Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator);
	// Resource creation
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateStaticBuffer(
		unsigned int dataStride,
		unsigned int dataCount,
		void* data);
	// Command list & synchronization
	void CloseExecuteAndResetCommandList();
	void WaitForGPU();

private:
	static DX12Utility* instance;
	// Overall device
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	// Command list related | Note: We're assuming a single command list for the entire engine
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	// Basic CPU/GPU synchronization
	Microsoft::WRL::ComPtr<ID3D12Fence> waitFence;
	HANDLE waitFenceEvent;
	unsigned long waitFenceCounter;

	DX12Utility() {};
};

