#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>

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

	~DX12Utility(){};

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

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCBVSRVDescriptorHeap();

	D3D12_GPU_DESCRIPTOR_HANDLE FillNextConstantBufferAndGetGPUDescriptorHandle(
		void* data,
		unsigned int dataSizeInBytes);

	D3D12_CPU_DESCRIPTOR_HANDLE LoadTexture(const wchar_t* filePath, bool generateMips = true);
	D3D12_GPU_DESCRIPTOR_HANDLE CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(
		D3D12_CPU_DESCRIPTOR_HANDLE firstDescriptorToCopy,
		unsigned int numDescriptorsToCopy);

private:
	static DX12Utility* instance;
	DX12Utility() :
		cbUploadHeap(0),
		cbUploadHeapOffsetInBytes(0),
		cbUploadHeapSizeInBytes(0),
		cbUploadHeapStartAddress(0),
		cbvSrvDescriptorHeap(0),
		cbvSrvDescriptorHeapIncrementSize(0),
		cbvDescriptorOffset(0),
		waitFenceCounter(0),
		waitFenceEvent(0),
		waitFence(0)
	{ };

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

	// Maximum number of constant buffers, assuming each buffer
	// is 256 bytes or less. Larger buffers are fine, but will
	// result in fewer buffers in use at any time
	const unsigned int maxConstantBuffers = 1000;

	// GPU-side constant buffer upload heap
	Microsoft::WRL::ComPtr<ID3D12Resource> cbUploadHeap;
	UINT64 cbUploadHeapSizeInBytes;
	UINT64 cbUploadHeapOffsetInBytes;
	void* cbUploadHeapStartAddress;

	// GPU-side CBV/SRV descriptor heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvSrvDescriptorHeap;
	SIZE_T cbvSrvDescriptorHeapIncrementSize;
	unsigned int cbvDescriptorOffset;

	// Maximum number of texture descriptors (SRVs) we can have.
	// Each material will have a chunk of this,
	// Note: If we delayed the creation of this heap until
	// after all textures and materials were created,
	// we could come up with an exact amount. The following
	// constant ensures we (hopefully) never run out of room.
	const unsigned int maxTextureDescriptors = 1000;
	unsigned int srvDescriptorOffset;
	// Texture resources we need to keep alive
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textures;
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> cpuSideTextureDescriptorHeaps;

	void CreateConstantBufferUploadHeap();
	void CreateCBVSRVDescriptorHeap();
};

