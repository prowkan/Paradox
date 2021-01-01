#include "RenderSystem.h"

#include <Core/Application.h>

void RenderSystem::InitSystem()
{
	HRESULT hr;

	UINT FactoryCreationFlags = 0;
	ULONG RefCount;

#ifdef _DEBUG
	ID3D12Debug3 *Debug;
	hr = D3D12GetDebugInterface(__uuidof(ID3D12Debug3), (void**)&Debug);
	Debug->EnableDebugLayer();
	Debug->SetEnableGPUBasedValidation(TRUE);
	FactoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	IDXGIFactory7 *Factory;

	hr = CreateDXGIFactory2(FactoryCreationFlags, __uuidof(IDXGIFactory7), (void**)&Factory);

	hr = Factory->MakeWindowAssociation(Application::GetMainWindowHandle(), DXGI_MWA_NO_ALT_ENTER);

	IDXGIAdapter *Adapter;

	hr = Factory->EnumAdapters(0, (IDXGIAdapter**)&Adapter);

	hr = D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&Device);

	RefCount = Adapter->Release();

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask = 0;
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr = Device->CreateCommandQueue(&CommandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&CommandQueue);

	hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&CommandAllocators[0]);
	hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&CommandAllocators[1]);

	hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0], nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&CommandList);
	hr = CommandList->Close();

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG::DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	SwapChainDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.Height = 1080;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
	SwapChainDesc.Stereo = FALSE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	SwapChainDesc.Width = 1920;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC SwapChainFullScreenDesc;
	SwapChainFullScreenDesc.RefreshRate.Numerator = 165003;
	SwapChainFullScreenDesc.RefreshRate.Denominator = 100;
	SwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainFullScreenDesc.Windowed = TRUE;

	IDXGISwapChain1 *SwapChain1;
	hr = Factory->CreateSwapChainForHwnd(CommandQueue, Application::GetMainWindowHandle(), &SwapChainDesc, &SwapChainFullScreenDesc, nullptr, &SwapChain1);
	hr = SwapChain1->QueryInterface<IDXGISwapChain4>(&SwapChain);

	RefCount = SwapChain1->Release();

	RefCount = Factory->Release();

	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 2;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	
	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&RTDescriptorHeap);

	hr = SwapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&BackBufferTextures[0]);
	hr = SwapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&BackBufferTextures[1]);

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	BackBufferRTVs[0].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 0 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	BackBufferRTVs[1].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 1 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	Device->CreateRenderTargetView(BackBufferTextures[0], &RTVDesc, BackBufferRTVs[0]);
	Device->CreateRenderTargetView(BackBufferTextures[1], &RTVDesc, BackBufferRTVs[1]);

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CurrentFrameIndex = 0;

	hr = Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Fences[0]);
	hr = Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Fences[1]);

	Event = CreateEvent(NULL, FALSE, FALSE, L"Event");
}

void RenderSystem::ShutdownSystem()
{
	HRESULT hr;

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;

	if (Fences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		hr = Fences[CurrentFrameIndex]->SetEventOnCompletion(1, Event);
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	ULONG RefCount;

	RefCount = BackBufferTextures[0]->Release();
	RefCount = BackBufferTextures[1]->Release();

	RefCount = Fences[0]->Release();
	RefCount = Fences[1]->Release();

	BOOL Result;

	Result = CloseHandle(Event);

	RefCount = RTDescriptorHeap->Release();

	RefCount = CommandList->Release();

	RefCount = CommandAllocators[0]->Release();
	RefCount = CommandAllocators[1]->Release();

	RefCount = SwapChain->Release();

	RefCount = CommandQueue->Release();

	RefCount = Device->Release();
}

void RenderSystem::TickSystem(float DeltaTime)
{
	HRESULT hr;

	hr = CommandAllocators[CurrentFrameIndex]->Reset();
	hr = CommandList->Reset(CommandAllocators[CurrentFrameIndex], nullptr);

	D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = BackBufferTextures[CurrentBackBufferIndex];
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	float ClearColor[4] = { 0.0f, 0.5f, 1.0f, 1.0f };
	CommandList->ClearRenderTargetView(BackBufferRTVs[CurrentBackBufferIndex], ClearColor, 0, nullptr);

	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = BackBufferTextures[CurrentBackBufferIndex];
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	hr = CommandList->Close();

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	hr = SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);

	hr = CommandQueue->Signal(Fences[CurrentFrameIndex], 1);

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	if (Fences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		hr = Fences[CurrentFrameIndex]->SetEventOnCompletion(1, Event);
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	hr = Fences[CurrentFrameIndex]->Signal(0);
}