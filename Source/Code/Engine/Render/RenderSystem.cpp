#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/GameObjects/Render/Meshes/StaticMeshObject.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

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

	IDXGIAdapter *Adapter;

	hr = Factory->EnumAdapters(0, (IDXGIAdapter**)&Adapter);

	IDXGIOutput *Monitor;

	hr = Adapter->EnumOutputs(0, &Monitor);

	UINT DisplayModesCount;
	hr = Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, nullptr);
	DXGI_MODE_DESC *DisplayModes = new DXGI_MODE_DESC[DisplayModesCount];
	hr = Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, DisplayModes);

	RefCount = Monitor->Release();

	ResolutionWidth = DisplayModes[DisplayModesCount - 1].Width;
	ResolutionHeight = DisplayModes[DisplayModesCount - 1].Height;

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
	SwapChainDesc.Height = ResolutionHeight;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.Scaling = DXGI_SCALING::DXGI_SCALING_STRETCH;
	SwapChainDesc.Stereo = FALSE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	SwapChainDesc.Width = ResolutionWidth;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC SwapChainFullScreenDesc;
	SwapChainFullScreenDesc.RefreshRate.Numerator = DisplayModes[DisplayModesCount - 1].RefreshRate.Numerator;
	SwapChainFullScreenDesc.RefreshRate.Denominator = DisplayModes[DisplayModesCount - 1].RefreshRate.Denominator;
	SwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainFullScreenDesc.Windowed = TRUE;

	IDXGISwapChain1 *SwapChain1;
	hr = Factory->CreateSwapChainForHwnd(CommandQueue, Application::GetMainWindowHandle(), &SwapChainDesc, &SwapChainFullScreenDesc, nullptr, &SwapChain1);
	hr = SwapChain1->QueryInterface<IDXGISwapChain4>(&SwapChain);

	hr = Factory->MakeWindowAssociation(Application::GetMainWindowHandle(), DXGI_MWA_NO_ALT_ENTER);

	RefCount = SwapChain1->Release();

	RefCount = Factory->Release();

	delete[] DisplayModes;

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CurrentFrameIndex = 0;

	hr = Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Fences[0]);
	hr = Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Fences[1]);

	Event = CreateEvent(NULL, FALSE, FALSE, L"Event");

	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 2;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&RTDescriptorHeap);

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&DSDescriptorHeap);

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&CBSRUADescriptorHeap);

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&SamplersDescriptorHeap);

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 100000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&ConstantBufferDescriptorHeap);

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 4000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&TexturesDescriptorHeap);

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 100000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameResourcesDescriptorHeaps[0]);
	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameResourcesDescriptorHeaps[1]);

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 2000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameSamplersDescriptorHeaps[0]);
	hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameSamplersDescriptorHeaps[1]);

	D3D12_DESCRIPTOR_RANGE DescriptorRanges[3];
	DescriptorRanges[0].BaseShaderRegister = 0;
	DescriptorRanges[0].NumDescriptors = 1;
	DescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescriptorRanges[0].RegisterSpace = 0;
	DescriptorRanges[1].BaseShaderRegister = 0;
	DescriptorRanges[1].NumDescriptors = 1;
	DescriptorRanges[1].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescriptorRanges[1].RegisterSpace = 0;
	DescriptorRanges[2].BaseShaderRegister = 0;
	DescriptorRanges[2].NumDescriptors = 1;
	DescriptorRanges[2].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	DescriptorRanges[2].RegisterSpace = 0;

	D3D12_ROOT_PARAMETER RootParameters[6];
	RootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[0].DescriptorTable.pDescriptorRanges = &DescriptorRanges[0];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
	RootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[1].DescriptorTable.pDescriptorRanges = &DescriptorRanges[1];
	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
	RootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[2].DescriptorTable.pDescriptorRanges = &DescriptorRanges[2];
	RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
	RootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[3].DescriptorTable.pDescriptorRanges = &DescriptorRanges[0];
	RootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
	RootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[4].DescriptorTable.pDescriptorRanges = &DescriptorRanges[1];
	RootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
	RootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[5].DescriptorTable.pDescriptorRanges = &DescriptorRanges[2];
	RootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc;
	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	RootSignatureDesc.NumParameters = 6;
	RootSignatureDesc.NumStaticSamplers = 0;
	RootSignatureDesc.pParameters = RootParameters;
	RootSignatureDesc.pStaticSamplers = nullptr;

	ID3DBlob *RootSignatureBlob;

	hr = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSignatureBlob, nullptr);

	hr = Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&RootSignature);

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

	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, __uuidof(ID3D12Resource), (void**)&DepthBufferTexture);

	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	DSVDesc.Texture2D.MipSlice = 0;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

	DepthBufferDSV.ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 0 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	Device->CreateDepthStencilView(DepthBufferTexture, &DSVDesc, DepthBufferDSV);

	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 256 * 20000;

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, __uuidof(ID3D12Resource), (void**)&GPUConstantBuffer);

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&CPUConstantBuffers[0]);
	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&CPUConstantBuffers[1]);

	for (int i = 0; i < 20000; i++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUConstantBuffer->GetGPUVirtualAddress() + i * 256;
		CBVDesc.SizeInBytes = 256;
		
		ConstantBufferCBVs[i].ptr = ConstantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + i * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		Device->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs[i]);
	}

	D3D12_SAMPLER_DESC SamplerDesc;
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_ANISOTROPIC;
	SamplerDesc.MaxAnisotropy = 16;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	Sampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 0 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	Device->CreateSampler(&SamplerDesc, Sampler);	
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

	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	{
		RefCount = renderMesh->VertexBuffer->Release();
		RefCount = renderMesh->IndexBuffer->Release();

		delete renderMesh;
	}

	RenderMeshDestructionQueue.clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	{
		RefCount = renderMaterial->PipelineState->Release();

		delete renderMaterial;
	}

	RenderMaterialDestructionQueue.clear();

	for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	{
		RefCount = renderTexture->Texture->Release();

		delete renderTexture;
	}

	RenderTextureDestructionQueue.clear();

	RefCount = BackBufferTextures[0]->Release();
	RefCount = BackBufferTextures[1]->Release();

	RefCount = DepthBufferTexture->Release();

	RefCount = Fences[0]->Release();
	RefCount = Fences[1]->Release();

	BOOL Result;

	Result = CloseHandle(Event);

	RefCount = GPUConstantBuffer->Release();
	RefCount = CPUConstantBuffers[0]->Release();
	RefCount = CPUConstantBuffers[1]->Release();

	RefCount = RootSignature->Release();

	RefCount = RTDescriptorHeap->Release();
	RefCount = DSDescriptorHeap->Release();
	RefCount = CBSRUADescriptorHeap->Release();
	RefCount = SamplersDescriptorHeap->Release();

	RefCount = ConstantBufferDescriptorHeap->Release();
	RefCount = TexturesDescriptorHeap->Release();

	RefCount = FrameResourcesDescriptorHeaps[0]->Release();
	RefCount = FrameResourcesDescriptorHeaps[1]->Release();

	RefCount = FrameSamplersDescriptorHeaps[0]->Release();
	RefCount = FrameSamplersDescriptorHeaps[1]->Release();

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

	D3D12_CPU_DESCRIPTOR_HANDLE ResourceCPUHandle = FrameResourcesDescriptorHeaps[CurrentFrameIndex]->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE ResourceGPUHandle = FrameResourcesDescriptorHeaps[CurrentFrameIndex]->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE SamplerCPUHandle = FrameSamplersDescriptorHeaps[CurrentFrameIndex]->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE SamplerGPUHandle = FrameSamplersDescriptorHeaps[CurrentFrameIndex]->GetGPUDescriptorHandleForHeapStart();
	UINT ResourceHandleSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	UINT SamplerHandleSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	ID3D12DescriptorHeap *DescriptorHeaps[2] = { FrameResourcesDescriptorHeaps[CurrentFrameIndex], FrameSamplersDescriptorHeaps[CurrentFrameIndex] };

	CommandList->SetDescriptorHeaps(2, DescriptorHeaps);
	CommandList->SetGraphicsRootSignature(RootSignature);

	XMMATRIX ViewProjMatrix = XMMatrixLookToLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)) * XMMatrixPerspectiveFovLH(3.14f / 2.0f, 16.0f / 9.0f, 0.01f, 1000.0f);

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	void *ConstantBufferData;
	SIZE_T ConstantBufferOffset = 0;

	hr = CPUConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData);

	vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
	size_t AllStaticMeshComponentsCount = AllStaticMeshComponents.size();

	for (int k = 0; k < AllStaticMeshComponentsCount; k++)
	{
		XMFLOAT3 Location = AllStaticMeshComponents[k]->GetTransformComponent()->GetLocation();
		XMFLOAT3 Rotation = AllStaticMeshComponents[k]->GetTransformComponent()->GetRotation();
		XMFLOAT3 Scale = AllStaticMeshComponents[k]->GetTransformComponent()->GetScale();

		XMMATRIX WorldMatrix = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z) * XMMatrixScaling(Scale.x, Scale.y, Scale.z) * XMMatrixTranslation(Location.x, Location.y, Location.z);
		XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;

		memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset, &WVPMatrix, sizeof(XMMATRIX));

		ConstantBufferOffset += 256;
	}

	WrittenRange.Begin = 0;
	WrittenRange.End = ConstantBufferOffset;

	CPUConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

	D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = GPUConstantBuffer;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	CommandList->CopyBufferRegion(GPUConstantBuffer, 0, CPUConstantBuffers[CurrentFrameIndex], 0, ConstantBufferOffset);

	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = GPUConstantBuffer;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = BackBufferTextures[CurrentBackBufferIndex];
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	CommandList->OMSetRenderTargets(1, &BackBufferRTVs[CurrentBackBufferIndex], TRUE, &DepthBufferDSV);

	CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = float(ResolutionHeight);
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(ResolutionWidth);

	CommandList->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = ResolutionHeight;
	ScissorRect.left = 0;
	ScissorRect.right = ResolutionWidth;
	ScissorRect.top = 0;

	CommandList->RSSetScissorRects(1, &ScissorRect);

	Device->CopyDescriptorsSimple(1, SamplerCPUHandle, Sampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SamplerCPUHandle.ptr += SamplerHandleSize;

	CommandList->SetGraphicsRootDescriptorTable(5, D3D12_GPU_DESCRIPTOR_HANDLE{ SamplerGPUHandle.ptr + 0 * ResourceHandleSize });
	SamplerGPUHandle.ptr += SamplerHandleSize;

	float ClearColor[4] = { 0.0f, 0.5f, 1.0f, 1.0f };
	CommandList->ClearRenderTargetView(BackBufferRTVs[CurrentBackBufferIndex], ClearColor, 0, nullptr);
	CommandList->ClearDepthStencilView(DepthBufferDSV, D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	for (int k = 0; k < AllStaticMeshComponentsCount; k++)
	{
		StaticMeshComponent *staticMeshComponent = AllStaticMeshComponents[k];

		RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
		RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
		RenderTexture *renderTexture = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();

		Device->CopyDescriptorsSimple(1, ResourceCPUHandle, ConstantBufferCBVs[k], D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		ResourceCPUHandle.ptr += ResourceHandleSize;
		Device->CopyDescriptorsSimple(1, ResourceCPUHandle, renderTexture->TextureSRV, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		ResourceCPUHandle.ptr += ResourceHandleSize;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		VertexBufferView.BufferLocation = renderMesh->VertexBuffer->GetGPUVirtualAddress();
		VertexBufferView.SizeInBytes = sizeof(Vertex) * 9 * 9 * 6;
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		IndexBufferView.BufferLocation = renderMesh->IndexBuffer->GetGPUVirtualAddress();
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * 8 * 8 * 6 * 6;

		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		CommandList->IASetIndexBuffer(&IndexBufferView);

		CommandList->SetPipelineState(renderMaterial->PipelineState);

		CommandList->SetGraphicsRootDescriptorTable(0, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->DrawIndexedInstanced(8 * 8 * 6 * 6, 1, 0, 0, 0);
	}

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

RenderMesh* RenderSystem::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMesh *renderMesh = new RenderMesh();

	HRESULT hr;

	ID3D12Resource *TemporaryVertexBuffer, *TemporaryIndexBuffer;

	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = sizeof(Vertex) * renderMeshCreateInfo.VertexCount;

	D3D12_HEAP_PROPERTIES HeapProperties;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&renderMesh->VertexBuffer);

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&TemporaryVertexBuffer);

	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = sizeof(WORD) * renderMeshCreateInfo.IndexCount;

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&renderMesh->IndexBuffer);

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&TemporaryIndexBuffer);

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * renderMeshCreateInfo.VertexCount;

	hr = TemporaryVertexBuffer->Map(0, &ReadRange, &MappedData);
	memcpy(MappedData, renderMeshCreateInfo.VertexData, sizeof(Vertex) * renderMeshCreateInfo.VertexCount);
	TemporaryVertexBuffer->Unmap(0, &WrittenRange);

	ReadRange.Begin = ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(WORD) * renderMeshCreateInfo.IndexCount;

	hr = TemporaryIndexBuffer->Map(0, &ReadRange, &MappedData);
	memcpy(MappedData, renderMeshCreateInfo.IndexData, sizeof(WORD) * renderMeshCreateInfo.IndexCount);
	TemporaryIndexBuffer->Unmap(0, &WrittenRange);

	hr = CommandAllocators[0]->Reset();
	hr = CommandList->Reset(CommandAllocators[0], nullptr);

	CommandList->CopyBufferRegion(renderMesh->VertexBuffer, 0, TemporaryVertexBuffer, 0, sizeof(Vertex) * renderMeshCreateInfo.VertexCount);
	CommandList->CopyBufferRegion(renderMesh->IndexBuffer, 0, TemporaryIndexBuffer, 0, sizeof(WORD) * renderMeshCreateInfo.IndexCount);

	D3D12_RESOURCE_BARRIER ResourceBarriers[2];
	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = renderMesh->VertexBuffer;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = renderMesh->IndexBuffer;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(2, ResourceBarriers);

	hr = CommandList->Close();

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	hr = CommandQueue->Signal(Fences[0], 2);

	if (Fences[0]->GetCompletedValue() != 2)
	{
		hr = Fences[0]->SetEventOnCompletion(2, Event);
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	hr = Fences[0]->Signal(1);

	ULONG RefCount;

	RefCount = TemporaryVertexBuffer->Release();
	RefCount = TemporaryIndexBuffer->Release();

	return renderMesh;
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTexture *renderTexture = new RenderTexture();

	HRESULT hr;

	ID3D12Resource *TemporaryTexture;

	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = renderTextureCreateInfo.SRGB ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	ResourceDesc.Height = renderTextureCreateInfo.Height;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = renderTextureCreateInfo.MIPLevels;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = renderTextureCreateInfo.Width;

	D3D12_HEAP_PROPERTIES HeapProperties;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&renderTexture->Texture);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrints[16];

	UINT NumsRows[16];
	UINT64 RowsSizesInBytes[16], TotalBytes;

	Device->GetCopyableFootprints(&ResourceDesc, 0, renderTextureCreateInfo.MIPLevels, 0, PlacedSubResourceFootPrints, NumsRows, RowsSizesInBytes, &TotalBytes);

	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = TotalBytes;

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	hr = Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&TemporaryTexture);

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = TotalBytes;

	hr = TemporaryTexture->Map(0, &ReadRange, &MappedData);

	BYTE *TexelData = renderTextureCreateInfo.TexelData;

	for (int i = 0; i < 8; i++)
	{
		for (UINT j = 0; j < NumsRows[i]; j++)
		{
			memcpy((BYTE*)MappedData + PlacedSubResourceFootPrints[i].Offset + j * PlacedSubResourceFootPrints[i].Footprint.RowPitch, (BYTE*)TexelData + j * RowsSizesInBytes[i], RowsSizesInBytes[i]);
		}

		TexelData += 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
	}

	TemporaryTexture->Unmap(0, &WrittenRange);

	hr = CommandAllocators[0]->Reset();
	hr = CommandList->Reset(CommandAllocators[0], nullptr);

	D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

	SourceTextureCopyLocation.pResource = TemporaryTexture;
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	DestTextureCopyLocation.pResource = renderTexture->Texture;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	for (int i = 0; i < 8; i++)
	{
		SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrints[i];
		DestTextureCopyLocation.SubresourceIndex = i;

		CommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);
	}

	D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = renderTexture->Texture;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	hr = CommandList->Close();

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	hr = CommandQueue->Signal(Fences[0], 2);

	if (Fences[0]->GetCompletedValue() != 2)
	{
		hr = Fences[0]->SetEventOnCompletion(2, Event);
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	hr = Fences[0]->Signal(1);

	ULONG RefCount = TemporaryTexture->Release();

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = renderTextureCreateInfo.SRGB ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 8;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	renderTexture->TextureSRV.ptr = TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + TexturesDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	++TexturesDescriptorsCount;

	Device->CreateShaderResourceView(renderTexture->Texture, &SRVDesc, renderTexture->TextureSRV);

	return renderTexture;
}

RenderMaterial* RenderSystem::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterial *renderMaterial = new RenderMaterial();

	HRESULT hr;

	D3D12_INPUT_ELEMENT_DESC InputElementDescs[2];
	InputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 0;
	InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 2;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = RootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = renderMaterialCreateInfo.PixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = renderMaterialCreateInfo.PixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = renderMaterialCreateInfo.VertexShaderByteCodeLength;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = renderMaterialCreateInfo.VertexShaderByteCodeData;

	hr = Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&renderMaterial->PipelineState);

	return renderMaterial;
}

void RenderSystem::DestroyRenderMesh(RenderMesh* renderMesh)
{
	RenderMeshDestructionQueue.push_back(renderMesh);
}

void RenderSystem::DestroyRenderTexture(RenderTexture* renderTexture)
{
	RenderTextureDestructionQueue.push_back(renderTexture);
}

void RenderSystem::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	RenderMaterialDestructionQueue.push_back(renderMaterial);
}