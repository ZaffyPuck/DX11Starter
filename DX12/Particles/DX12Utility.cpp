#include "WICTextureLoader.h"		//DXTK
#include "ResourceUploadBatch.h"	//DXTK
#include "DX12Utility.h"

using namespace DirectX;

DX12Utility* DX12Utility::instance;

// --------------------------------------------------------
// Sets up the helper with required DX12 objects
// --------------------------------------------------------
void DX12Utility::Initialize(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator)
{
	// Save objects
	this->device = device;
	this->commandList = commandList;
	this->commandQueue = commandQueue;
	this->commandAllocator = commandAllocator;

	// Create the fence for basic synchronization
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(waitFence.GetAddressOf()));
	waitFenceEvent = CreateEventEx(0, 0, 0, EVENT_ALL_ACCESS);
	waitFenceCounter = 0;

	CreateConstantBufferUploadHeap();
	CreateCBVSRVDescriptorHeap();
}

// --------------------------------------------------------
// Closes the current command list and tells the GPU to start executing those commands.
// We also wait for the GPU to finish this work so we can reset the command allocator
// (which CANNOT be reset while the GPU is using its commands) and the command list itself.
// --------------------------------------------------------
void DX12Utility::CloseExecuteAndResetCommandList()
{
	// Close the current list and execute it as our only list
	commandList->Close();
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, lists);

	// Always wait before reseting command allocator, as it should not
	// be reset while the GPU is processing a command list
	// See: https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12commandallocator-reset
	WaitForGPU();
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), 0);
}

// --------------------------------------------------------
// Makes our C++ code wait for the GPU to finish its
// current batch of work before moving on.
// --------------------------------------------------------
void DX12Utility::WaitForGPU()
{
	// Update our ongoing fence value (a unique index for each "stop sign")
	// and then place that value into the GPU's command queue
	waitFenceCounter++;
	commandQueue->Signal(waitFence.Get(), waitFenceCounter);

	// Check to see if the most recently completed fence value
	// is less than the one we just set.
	if (waitFence->GetCompletedValue() < waitFenceCounter)
	{
		// Tell the fence to let us know when it's hit, and then
		// sit an wait until that fence is hit.
		waitFence->SetEventOnCompletion(waitFenceCounter, waitFenceEvent);
		WaitForSingleObject(waitFenceEvent, INFINITE);
	}
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DX12Utility::GetCBVSRVDescriptorHeap()
{
	return cbvSrvDescriptorHeap;
}

// --------------------------------------------------------
// Copies the given data into the next "unused" spot in the CBV upload heap (wrapping at the end, since
// we treat it like a ring buffer). Then creates a CBV in the next "unused" spot in the CBV heap that
// points to the aforementioned spot in the upload heap and returns that CBV (a GPU descriptor handle).
//
// data - The data to copy to the GPU
// dataSizeInBytes - The byte size of the data to copy
// --------------------------------------------------------
D3D12_GPU_DESCRIPTOR_HANDLE DX12Utility::FillNextConstantBufferAndGetGPUDescriptorHandle(void* data, unsigned int dataSizeInBytes)
{
	// How much space will we need? Each CBV must point to a chunk of the upload heap that is
	// a multiple of 256 bytes, so we need to calculate and reserve that amount.
	SIZE_T reservationSize = (SIZE_T)dataSizeInBytes;
	reservationSize = (reservationSize + 255) / 256 * 256; // Integer division trick
	// Ensure this upload will fit in the remaining space. If not, reset to beginning.

	if (cbUploadHeapOffsetInBytes + reservationSize >= cbUploadHeapSizeInBytes)
	{
		cbUploadHeapOffsetInBytes = 0;
	}

	// Where in the upload heap will this data go?
	D3D12_GPU_VIRTUAL_ADDRESS virtualGPUAddress = cbUploadHeap->GetGPUVirtualAddress() + cbUploadHeapOffsetInBytes;

	// === Copy data to the upload heap ===
	{
		// Calculate the actual upload address (which we got from mapping the buffer)
		// Note that this is different than the GPU virtual address needed for the CBV below
		void* uploadAddress = reinterpret_cast<void*>(
			(SIZE_T)cbUploadHeapStartAddress + cbUploadHeapOffsetInBytes);

		// Perform the mem copy to put new data into this part of the heap
		memcpy(uploadAddress, data, dataSizeInBytes);

		// Increment the offset and loop back to the beginning if necessary,
		// allowing us to treat the upload heap like a ring buffer
		cbUploadHeapOffsetInBytes += reservationSize;
		if (cbUploadHeapOffsetInBytes >= cbUploadHeapSizeInBytes)
		{
			cbUploadHeapOffsetInBytes = 0;
		}
	}
	// Create a CBV for this section of the heap
	{
		// Calculate the CPU and GPU side handles for this descriptor
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = cbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = cbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		// Offset each by based on how many descriptors we've used
		// Note: cbvDescriptorOffset is a COUNT of descriptors, not bytes so we must calculate the size
		cpuHandle.ptr += (SIZE_T)cbvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;
		gpuHandle.ptr += (SIZE_T)cbvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;

		// Describe the constant buffer view that points to our latest chunk of the CB upload heap
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = virtualGPUAddress;
		cbvDesc.SizeInBytes = (UINT)reservationSize;

		// Create the CBV, which is a lightweight operation in DX12
		device->CreateConstantBufferView(&cbvDesc, cpuHandle);

		// Increment the offset and loop back to the beginning if necessary
		// which allows us to treat the descriptor heap as a ring buffer
		cbvDescriptorOffset++;
		if (cbvDescriptorOffset >= maxConstantBuffers)
		{
			cbvDescriptorOffset = 0;
		}

		// Now that the CBV is ready, we return the GPU handle to it
		// so it can be set as part of the root signature during drawing
		return gpuHandle;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Utility::LoadTexture(const wchar_t* filePath, bool generateMips)
{
	// Helper function from DXTK for uploading a resource
	// (like a texture) to the appropriate GPU memory
	ResourceUploadBatch upload(device.Get());
	upload.Begin();

	// Attempt to create the texture
	Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	CreateWICTextureFromFile(device.Get(), upload, filePath, texture.GetAddressOf(), generateMips);

	// Perform the upload and wait for it to finish before returning the texture
	auto finish = upload.End(commandQueue.Get());
	finish.wait();

	// Now that we have the texture, add to our list and make a CPU-side descriptor heap
	// just for this texture's SRV. Note that it would probably be better to put all
	// texture SRVs into the same descriptor heap, but we don't know how many we'll need
	// until they're all loaded and this is a quick and dirty implementation!
	textures.push_back(texture);

	// Create the CPU-SIDE descriptor heap for our descriptor
	D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
	dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Non-shader visible for CPU-side-only desc heap
	dhDesc.NodeMask = 0;
	dhDesc.NumDescriptors = 1;
	dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descHeap;
	device->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(descHeap.GetAddressOf()));
	cpuSideTextureDescriptorHeaps.push_back(descHeap);

	// Create the SRV on this descriptor heap
	// Note: Using a null description results in the "default" SRV (same format, all mips, all array slices, etc.)
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateShaderResourceView(texture.Get(), 0, cpuHandle);

	// Return the CPU descriptor handle, which can be used to
	// copy the descriptor to a shader-visible heap later
	return cpuHandle;

}

// The goal of this function is to copy one or more CPU - side(non - shader - visible) descriptors to the final
// descriptor heap that we use when drawing(the CBV / SRV descriptor heap) and return the handle to the
// location of the first one.
D3D12_GPU_DESCRIPTOR_HANDLE DX12Utility::CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(
	D3D12_CPU_DESCRIPTOR_HANDLE firstDescriptorToCopy,
	unsigned int numDescriptorsToCopy)
{
	// Grab the actual heap start on both sides and offset to the next open SRV portion
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = cbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = cbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	cpuHandle.ptr += (SIZE_T)srvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;
	gpuHandle.ptr += (SIZE_T)srvDescriptorOffset * cbvSrvDescriptorHeapIncrementSize;

	// We know where to copy these descriptors, so copy all of them and remember the new offset
	device->CopyDescriptorsSimple(
		numDescriptorsToCopy,
		cpuHandle,
		firstDescriptorToCopy,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvDescriptorOffset += numDescriptorsToCopy;

	// Pass back the GPU handle to the start of this section
	// in the final CBV/SRV heap so the caller can use it later
	return gpuHandle;

}

// --------------------------------------------------------
// Creates a single CB upload heap which will store all
// constant buffer data for the entire program. This
// heap is treated as a ring buffer, allowing the program
// to continually re-use the memory as frames progress.
// --------------------------------------------------------
void DX12Utility::CreateConstantBufferUploadHeap()
{
	// This heap MUST have a size that is a multiple of 256
	// We'll support up to the max number of CBs if they're
	// all 256 bytes or less, or fewer overall CBs if they're larger
	cbUploadHeapSizeInBytes = maxConstantBuffers * 256;

	// Assume the first CB will start at the beginning of the heap
	// This offset changes as we use more CBs, and wraps around when full
	cbUploadHeapOffsetInBytes = 0;

	// Create the upload heap for our constant buffer
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // Upload heap since we'll be copying often
	heapProps.VisibleNodeMask = 1;

	// Fill out description
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Alignment = 0;
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Height = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Width = cbUploadHeapSizeInBytes; // Must be 256 byte aligned
	// Create a constant buffer resource heap

	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(cbUploadHeap.GetAddressOf()));

	// Keep mapped
	D3D12_RANGE range{ 0, 0 };
	cbUploadHeap->Map(0, &range, &cbUploadHeapStartAddress);
}

// --------------------------------------------------------
// Creates a single CBV descriptor heap which will store all
// CBVs and SRVs for the entire program. Like the CBV upload heap,
// this heap is treated as a ring buffer, allowing the program
// to continually re-use the memory as frames progress.
// --------------------------------------------------------
void DX12Utility::CreateCBVSRVDescriptorHeap()
{
	// Ask the device for the increment size for CBV descriptor heaps
	// This can vary by GPU so we need to query for it
	cbvSrvDescriptorHeapIncrementSize = (SIZE_T)device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Describe the descriptor heap we want to make
	D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
	dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Shaders can see these
	dhDesc.NodeMask = 0; // Node here means physical GPU - we only have 1 so its index is 0
	dhDesc.NumDescriptors = maxConstantBuffers; // How many descriptors will we need?
	dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // This heap can store CBVs, SRVs and UAVs
	dhDesc.NumDescriptors = maxConstantBuffers + maxTextureDescriptors;

	device->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(cbvSrvDescriptorHeap.GetAddressOf()));

	// Assume the first CBV will be at the beginning of the heap
	// This will increase as we use more CBVs and will wrap back to 0
	cbvDescriptorOffset = 0;

	// Assume the first SRV will be after all possible CBVs
	srvDescriptorOffset = maxConstantBuffers;
}

// --------------------------------------------------------
// Helper for creating a static buffer that will get
// data once and remain immutable
//
// dataStride - The size of one piece of data in the buffer (like a vertex)
// dataCount - How many pieces of data (like how many vertices)
// data - Pointer to the data itself
// --------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D12Resource> DX12Utility::CreateStaticBuffer(
	unsigned int dataStride,
	unsigned int dataCount,
	void* data)
{
	// The overall buffer we'll be creating
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer;

	// Describes the final heap
	D3D12_HEAP_PROPERTIES props = {};
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.CreationNodeMask = 1;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	props.Type = D3D12_HEAP_TYPE_DEFAULT;
	props.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC desc = {};
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Height = 1; // Assuming this is a regular buffer, not a texture
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Width = dataStride * dataCount; // Size of the buffer

	device->CreateCommittedResource(
		&props,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COPY_DEST, // Will eventually be "common", but we're copying first
		0,
		IID_PPV_ARGS(buffer.GetAddressOf()));

	// Now create an intermediate upload heap for copying initial data
	D3D12_HEAP_PROPERTIES uploadProps = {};
	uploadProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadProps.CreationNodeMask = 1;
	uploadProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD; // Can only ever be Generic_Read state
	uploadProps.VisibleNodeMask = 1;

	Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;
	device->CreateCommittedResource(
		&uploadProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(uploadHeap.GetAddressOf()));

	// Do a straight map/memcpy/unmap
	void* gpuAddress = 0;
	uploadHeap->Map(0, 0, &gpuAddress);
	memcpy(gpuAddress, data, dataStride * dataCount);
	uploadHeap->Unmap(0, 0);

	// Copy the whole buffer from uploadheap to vert buffer
	commandList->CopyResource(buffer.Get(), uploadHeap.Get());

	// Transition the buffer to generic read for the rest of the app lifetime (presumable)
	D3D12_RESOURCE_BARRIER rb = {};
	rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rb.Transition.pResource = buffer.Get();
	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &rb);

	// Execute the command list and return the finished buffer
	CloseExecuteAndResetCommandList();
	return buffer;
}