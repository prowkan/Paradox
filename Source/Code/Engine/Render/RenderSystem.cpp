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
	UINT FactoryCreationFlags = 0;

#ifdef _DEBUG
	ID3D12Debug3 *Debug;
	SAFE_DX(D3D12GetDebugInterface(__uuidof(ID3D12Debug3), (void**)&Debug));
	Debug->EnableDebugLayer();
	Debug->SetEnableGPUBasedValidation(TRUE);
	FactoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	IDXGIFactory7 *Factory;

	SAFE_DX(CreateDXGIFactory2(FactoryCreationFlags, __uuidof(IDXGIFactory7), (void**)&Factory));

	IDXGIAdapter *Adapter;

	SAFE_DX(Factory->EnumAdapters(0, (IDXGIAdapter**)&Adapter));

	IDXGIOutput *Monitor;

	SAFE_DX(Adapter->EnumOutputs(0, &Monitor));

	UINT DisplayModesCount;
	SAFE_DX(Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, nullptr));
	DXGI_MODE_DESC *DisplayModes = new DXGI_MODE_DESC[DisplayModesCount];
	SAFE_DX(Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, DisplayModes));

	SAFE_RELEASE(Monitor);

	ResolutionWidth = DisplayModes[DisplayModesCount - 1].Width;
	ResolutionHeight = DisplayModes[DisplayModesCount - 1].Height;

	SAFE_DX(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&Device));

	SAFE_RELEASE(Adapter);

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask = 0;
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

	SAFE_DX(Device->CreateCommandQueue(&CommandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&CommandQueue));

	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&CommandAllocators[0]));
	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&CommandAllocators[1]));

	SAFE_DX(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0], nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&CommandList));
	SAFE_DX(CommandList->Close());

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
	SAFE_DX(Factory->CreateSwapChainForHwnd(CommandQueue, Application::GetMainWindowHandle(), &SwapChainDesc, &SwapChainFullScreenDesc, nullptr, &SwapChain1));
	SAFE_DX(SwapChain1->QueryInterface<IDXGISwapChain4>(&SwapChain));

	SAFE_DX(Factory->MakeWindowAssociation(Application::GetMainWindowHandle(), DXGI_MWA_NO_ALT_ENTER));

	SAFE_RELEASE(SwapChain1);

	SAFE_RELEASE(Factory);

	delete[] DisplayModes;

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CurrentFrameIndex = 0;

	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Fences[0]));
	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&Fences[1]));

	Event = CreateEvent(NULL, FALSE, FALSE, (const wchar_t*)u"Event");

	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 2;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&RTDescriptorHeap));

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&DSDescriptorHeap));

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&CBSRUADescriptorHeap));

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 1;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&SamplersDescriptorHeap));

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 100000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&ConstantBufferDescriptorHeap));

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 4000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&TexturesDescriptorHeap));

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 100000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameResourcesDescriptorHeaps[0]));
	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameResourcesDescriptorHeaps[1]));

	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 2000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameSamplersDescriptorHeaps[0]));
	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&FrameSamplersDescriptorHeaps[1]));

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

	SAFE_DX(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSignatureBlob, nullptr));

	SAFE_DX(Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&RootSignature));

	SAFE_DX(SwapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&BackBufferTextures[0]));
	SAFE_DX(SwapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&BackBufferTextures[1]));

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

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, __uuidof(ID3D12Resource), (void**)&DepthBufferTexture));

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

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, __uuidof(ID3D12Resource), (void**)&GPUConstantBuffer));

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&CPUConstantBuffers[0]));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&CPUConstantBuffers[1]));

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

	D3D12_HEAP_DESC HeapDesc;
	HeapDesc.Alignment = 0;
	HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapDesc.Properties.CreationNodeMask = 0;
	HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapDesc.Properties.VisibleNodeMask = 0;
	HeapDesc.SizeInBytes = BUFFER_MEMORY_HEAP_SIZE;

	SAFE_DX(Device->CreateHeap(&HeapDesc, __uuidof(ID3D12Heap), (void**)&BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

	HeapDesc.Alignment = 0;
	HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapDesc.Properties.CreationNodeMask = 0;
	HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapDesc.Properties.VisibleNodeMask = 0;
	HeapDesc.SizeInBytes = TEXTURE_MEMORY_HEAP_SIZE;

	SAFE_DX(Device->CreateHeap(&HeapDesc, __uuidof(ID3D12Heap), (void**)&TextureMemoryHeaps[CurrentTextureMemoryHeapIndex]));

	HeapDesc.Alignment = 0;
	HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapDesc.Properties.CreationNodeMask = 0;
	HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapDesc.Properties.VisibleNodeMask = 0;
	HeapDesc.SizeInBytes = UPLOAD_HEAP_SIZE;

	SAFE_DX(Device->CreateHeap(&HeapDesc, __uuidof(ID3D12Heap), (void**)&UploadHeap));

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
	ResourceDesc.Width = UPLOAD_HEAP_SIZE;

	SAFE_DX(Device->CreatePlacedResource(UploadHeap, 0, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&UploadBuffer));
}

void RenderSystem::ShutdownSystem()
{
	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;

	if (Fences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		SAFE_DX(Fences[CurrentFrameIndex]->SetEventOnCompletion(1, Event));
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	{
		SAFE_RELEASE(renderMesh->VertexBuffer);
		SAFE_RELEASE(renderMesh->IndexBuffer);

		delete renderMesh;
	}

	RenderMeshDestructionQueue.clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	{
		SAFE_RELEASE(renderMaterial->PipelineState);

		delete renderMaterial;
	}

	RenderMaterialDestructionQueue.clear();

	for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	{
		SAFE_RELEASE(renderTexture->Texture);

		delete renderTexture;
	}

	RenderTextureDestructionQueue.clear();

	for (int i = 0; i < MAX_MEMORY_HEAPS_COUNT; i++)
	{
		if (BufferMemoryHeaps[i]) SAFE_RELEASE(BufferMemoryHeaps[i]);
		if (TextureMemoryHeaps[i]) SAFE_RELEASE(TextureMemoryHeaps[i]);
	}

	SAFE_RELEASE(UploadBuffer);
	SAFE_RELEASE(UploadHeap);

	SAFE_RELEASE(BackBufferTextures[0]);
	SAFE_RELEASE(BackBufferTextures[1]);

	SAFE_RELEASE(DepthBufferTexture);

	SAFE_RELEASE(Fences[0]);
	SAFE_RELEASE(Fences[1]);

	BOOL Result;

	Result = CloseHandle(Event);

	SAFE_RELEASE(GPUConstantBuffer);
	SAFE_RELEASE(CPUConstantBuffers[0]);
	SAFE_RELEASE(CPUConstantBuffers[1]);

	SAFE_RELEASE(RootSignature);

	SAFE_RELEASE(RTDescriptorHeap);
	SAFE_RELEASE(DSDescriptorHeap);
	SAFE_RELEASE(CBSRUADescriptorHeap);
	SAFE_RELEASE(SamplersDescriptorHeap);

	SAFE_RELEASE(ConstantBufferDescriptorHeap);
	SAFE_RELEASE(TexturesDescriptorHeap);

	SAFE_RELEASE(FrameResourcesDescriptorHeaps[0]);
	SAFE_RELEASE(FrameResourcesDescriptorHeaps[1]);

	SAFE_RELEASE(FrameSamplersDescriptorHeaps[0]);
	SAFE_RELEASE(FrameSamplersDescriptorHeaps[1]);

	SAFE_RELEASE(CommandList);

	SAFE_RELEASE(CommandAllocators[0]);
	SAFE_RELEASE(CommandAllocators[1]);

	SAFE_RELEASE(SwapChain);

	SAFE_RELEASE(CommandQueue);

	SAFE_RELEASE(Device);
}

void RenderSystem::TickSystem(float DeltaTime)
{
	SAFE_DX(CommandAllocators[CurrentFrameIndex]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[CurrentFrameIndex], nullptr));

	D3D12_CPU_DESCRIPTOR_HANDLE ResourceCPUHandle = FrameResourcesDescriptorHeaps[CurrentFrameIndex]->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE ResourceGPUHandle = FrameResourcesDescriptorHeaps[CurrentFrameIndex]->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE SamplerCPUHandle = FrameSamplersDescriptorHeaps[CurrentFrameIndex]->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE SamplerGPUHandle = FrameSamplersDescriptorHeaps[CurrentFrameIndex]->GetGPUDescriptorHandleForHeapStart();
	UINT ResourceHandleSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	UINT SamplerHandleSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	ID3D12DescriptorHeap *DescriptorHeaps[2] = { FrameResourcesDescriptorHeaps[CurrentFrameIndex], FrameSamplersDescriptorHeaps[CurrentFrameIndex] };

	CommandList->SetDescriptorHeaps(2, DescriptorHeaps);
	CommandList->SetGraphicsRootSignature(RootSignature);

	XMMATRIX ViewProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewProjMatrix();

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	void *ConstantBufferData;
	SIZE_T ConstantBufferOffset = 0;

	vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	OPTICK_EVENT("Draw Calls")

	SAFE_DX(CPUConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

	for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
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

	for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

		RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
		RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
		RenderTexture *renderTexture = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();

		UINT DestRangeSize = 2;
		UINT SourceRangeSizes[2] = { 1, 1 };
		D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { ConstantBufferCBVs[k], renderTexture->TextureSRV };

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		VertexBufferView.BufferLocation = renderMesh->VertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * 9 * 9 * 6;
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		IndexBufferView.BufferLocation = renderMesh->IndexBufferAddress;
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

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));

	SAFE_DX(CommandQueue->Signal(Fences[CurrentFrameIndex], 1));

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	if (Fences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		SAFE_DX(Fences[CurrentFrameIndex]->SetEventOnCompletion(1, Event));
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	SAFE_DX(Fences[CurrentFrameIndex]->Signal(0));
}

RenderMesh* RenderSystem::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMesh *renderMesh = new RenderMesh();

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

	D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	size_t AlignedResourceOffset = BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] + (ResourceAllocationInfo.Alignment - BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] % ResourceAllocationInfo.Alignment);

	if (AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes > BUFFER_MEMORY_HEAP_SIZE)
	{
		++CurrentBufferMemoryHeapIndex;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = BUFFER_MEMORY_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, __uuidof(ID3D12Heap), (void**)&BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

		AlignedResourceOffset = 0;
	}

	SAFE_DX(Device->CreatePlacedResource(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&renderMesh->VertexBuffer));

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes;

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

	ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	AlignedResourceOffset = BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] + (ResourceAllocationInfo.Alignment - BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] % ResourceAllocationInfo.Alignment);

	if (AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes > BUFFER_MEMORY_HEAP_SIZE)
	{
		++CurrentBufferMemoryHeapIndex;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = BUFFER_MEMORY_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, __uuidof(ID3D12Heap), (void**)&BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

		AlignedResourceOffset = 0;
	}

	SAFE_DX(Device->CreatePlacedResource(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&renderMesh->IndexBuffer));

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes;

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
	memcpy((BYTE*)MappedData, renderMeshCreateInfo.VertexData, sizeof(Vertex) * renderMeshCreateInfo.VertexCount);
	memcpy((BYTE*)MappedData + sizeof(Vertex) * renderMeshCreateInfo.VertexCount, renderMeshCreateInfo.IndexData, sizeof(WORD) * renderMeshCreateInfo.IndexCount);
	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CommandAllocators[0]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

	CommandList->CopyBufferRegion(renderMesh->VertexBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * renderMeshCreateInfo.VertexCount);
	CommandList->CopyBufferRegion(renderMesh->IndexBuffer, 0, UploadBuffer, sizeof(Vertex) * renderMeshCreateInfo.VertexCount, sizeof(WORD) * renderMeshCreateInfo.IndexCount);

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

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(CommandQueue->Signal(Fences[0], 2));

	if (Fences[0]->GetCompletedValue() != 2)
	{
		SAFE_DX(Fences[0]->SetEventOnCompletion(2, Event));
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	SAFE_DX(Fences[0]->Signal(1));

	renderMesh->VertexBufferAddress = renderMesh->VertexBuffer->GetGPUVirtualAddress();
	renderMesh->IndexBufferAddress = renderMesh->IndexBuffer->GetGPUVirtualAddress();

	return renderMesh;
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTexture *renderTexture = new RenderTexture();

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

	D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	size_t AlignedResourceOffset = TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] + (ResourceAllocationInfo.Alignment - TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] % ResourceAllocationInfo.Alignment);

	if (AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes > TEXTURE_MEMORY_HEAP_SIZE)
	{
		++CurrentTextureMemoryHeapIndex;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = TEXTURE_MEMORY_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, __uuidof(ID3D12Heap), (void**)&TextureMemoryHeaps[CurrentTextureMemoryHeapIndex]));

		AlignedResourceOffset = 0;
	}

	SAFE_DX(Device->CreatePlacedResource(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex], AlignedResourceOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&renderTexture->Texture));

	TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] = AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrints[16];

	UINT NumsRows[16];
	UINT64 RowsSizesInBytes[16], TotalBytes;

	Device->GetCopyableFootprints(&ResourceDesc, 0, renderTextureCreateInfo.MIPLevels, 0, PlacedSubResourceFootPrints, NumsRows, RowsSizesInBytes, &TotalBytes);

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = TotalBytes;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));

	BYTE *TexelData = renderTextureCreateInfo.TexelData;

	for (UINT i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
	{
		for (UINT j = 0; j < NumsRows[i]; j++)
		{
			memcpy((BYTE*)MappedData + PlacedSubResourceFootPrints[i].Offset + j * PlacedSubResourceFootPrints[i].Footprint.RowPitch, (BYTE*)TexelData + j * RowsSizesInBytes[i], RowsSizesInBytes[i]);
		}

		TexelData += 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
	}

	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CommandAllocators[0]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

	D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

	SourceTextureCopyLocation.pResource = UploadBuffer;
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	DestTextureCopyLocation.pResource = renderTexture->Texture;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	for (UINT i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
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

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(CommandQueue->Signal(Fences[0], 2));

	if (Fences[0]->GetCompletedValue() != 2)
	{
		SAFE_DX(Fences[0]->SetEventOnCompletion(2, Event));
		DWORD WaitResult = WaitForSingleObject(Event, INFINITE);
	}

	SAFE_DX(Fences[0]->Signal(1));

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = renderTextureCreateInfo.SRGB ? DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = renderTextureCreateInfo.MIPLevels;
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

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&renderMaterial->PipelineState));

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

void RenderSystem::CheckDXCallResult(HRESULT hr, const wchar_t* Function)
{
	if (FAILED(hr))
	{
		wchar_t DXErrorMessageBuffer[2048];
		wchar_t DXErrorCodeBuffer[512];

		const wchar_t *DXErrorCodePtr = GetDXErrorMessageFromHRESULT(hr);

		if (DXErrorCodePtr) wcscpy(DXErrorCodeBuffer, DXErrorCodePtr);
		else wsprintf(DXErrorCodeBuffer, (const wchar_t*)u"0x%08X (неизвестный код)", hr);

		wsprintf(DXErrorMessageBuffer, (const wchar_t*)u"Произошла ошибка при попытке вызова следующей DirectX-функции:\r\n%s\r\nКод ошибки: %s", Function, DXErrorCodeBuffer);

		int IntResult = MessageBox(NULL, DXErrorMessageBuffer, (const wchar_t*)u"Ошибка DirectX", MB_OK | MB_ICONERROR);

		if (hr == DXGI_ERROR_DEVICE_REMOVED) SAFE_DX(Device->GetDeviceRemovedReason());

		ExitProcess(0);
	}
}

const wchar_t* RenderSystem::GetDXErrorMessageFromHRESULT(HRESULT hr)
{
	switch (hr)
	{
		case E_UNEXPECTED:
			return (const wchar_t*)u"E_UNEXPECTED"; 
			break; 
		case E_NOTIMPL:
			return (const wchar_t*)u"E_NOTIMPL";
			break;
		case E_OUTOFMEMORY:
			return (const wchar_t*)u"E_OUTOFMEMORY";
			break; 
		case E_INVALIDARG:
			return (const wchar_t*)u"E_INVALIDARG";
			break; 
		case E_NOINTERFACE:
			return (const wchar_t*)u"E_NOINTERFACE";
			break; 
		case E_POINTER:
			return (const wchar_t*)u"E_POINTER"; 
			break; 
		case E_HANDLE:
			return (const wchar_t*)u"E_HANDLE"; 
			break;
		case E_ABORT:
			return (const wchar_t*)u"E_ABORT"; 
			break; 
		case E_FAIL:
			return (const wchar_t*)u"E_FAIL"; 
			break;
		case E_ACCESSDENIED:
			return (const wchar_t*)u"E_ACCESSDENIED";
			break; 
		case E_PENDING:
			return (const wchar_t*)u"E_PENDING";
			break; 
		case E_BOUNDS:
			return (const wchar_t*)u"E_BOUNDS";
			break; 
		case E_CHANGED_STATE:
			return (const wchar_t*)u"E_CHANGED_STATE";
			break; 
		case E_ILLEGAL_STATE_CHANGE:
			return (const wchar_t*)u"E_ILLEGAL_STATE_CHANGE";
			break; 
		case E_ILLEGAL_METHOD_CALL:
			return (const wchar_t*)u"E_ILLEGAL_METHOD_CALL";
			break; 
		case E_STRING_NOT_NULL_TERMINATED:
			return (const wchar_t*)u"E_STRING_NOT_NULL_TERMINATED"; 
			break; 
		case E_ILLEGAL_DELEGATE_ASSIGNMENT:
			return (const wchar_t*)u"E_ILLEGAL_DELEGATE_ASSIGNMENT";
			break; 
		case E_ASYNC_OPERATION_NOT_STARTED:
			return (const wchar_t*)u"E_ASYNC_OPERATION_NOT_STARTED"; 
			break; 
		case E_APPLICATION_EXITING:
			return (const wchar_t*)u"E_APPLICATION_EXITING";
			break; 
		case E_APPLICATION_VIEW_EXITING:
			return (const wchar_t*)u"E_APPLICATION_VIEW_EXITING";
			break; 
		case DXGI_ERROR_INVALID_CALL:
			return (const wchar_t*)u"DXGI_ERROR_INVALID_CALL";
			break; 
		case DXGI_ERROR_NOT_FOUND:
			return (const wchar_t*)u"DXGI_ERROR_NOT_FOUND";
			break; 
		case DXGI_ERROR_MORE_DATA:
			return (const wchar_t*)u"DXGI_ERROR_MORE_DATA";
			break; 
		case DXGI_ERROR_UNSUPPORTED:
			return (const wchar_t*)u"DXGI_ERROR_UNSUPPORTED";
			break; 
		case DXGI_ERROR_DEVICE_REMOVED:
			return (const wchar_t*)u"DXGI_ERROR_DEVICE_REMOVED";
			break; 
		case DXGI_ERROR_DEVICE_HUNG:
			return (const wchar_t*)u"DXGI_ERROR_DEVICE_HUNG";
			break; 
		case DXGI_ERROR_DEVICE_RESET:
			return (const wchar_t*)u"DXGI_ERROR_DEVICE_RESET";
			break; 
		case DXGI_ERROR_WAS_STILL_DRAWING:
			return (const wchar_t*)u"DXGI_ERROR_WAS_STILL_DRAWING";
			break; 
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
			return (const wchar_t*)u"DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
			break; 
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
			return (const wchar_t*)u"DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE"; 
			break; 
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			return (const wchar_t*)u"DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break; 
		case DXGI_ERROR_NONEXCLUSIVE:
			return (const wchar_t*)u"DXGI_ERROR_NONEXCLUSIVE";
			break; 
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			return (const wchar_t*)u"DXGI_ERROR_NOT_CURRENTLY_AVAILABLE"; 
			break; 
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
			return (const wchar_t*)u"DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
			break; 
		case DXGI_ERROR_REMOTE_OUTOFMEMORY:
			return (const wchar_t*)u"DXGI_ERROR_REMOTE_OUTOFMEMORY"; 
			break; 
		case DXGI_ERROR_ACCESS_LOST:
			return (const wchar_t*)u"DXGI_ERROR_ACCESS_LOST";
			break; 
		case DXGI_ERROR_WAIT_TIMEOUT:
			return (const wchar_t*)u"DXGI_ERROR_WAIT_TIMEOUT";
			break; 
		case DXGI_ERROR_SESSION_DISCONNECTED:
			return (const wchar_t*)u"DXGI_ERROR_SESSION_DISCONNECTED";
			break; 
		case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
			return (const wchar_t*)u"DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";
			break; 
		case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
			return (const wchar_t*)u"DXGI_ERROR_CANNOT_PROTECT_CONTENT";
			break; 
		case DXGI_ERROR_ACCESS_DENIED:
			return (const wchar_t*)u"DXGI_ERROR_ACCESS_DENIED"; 
			break; 
		case DXGI_ERROR_NAME_ALREADY_EXISTS:
			return (const wchar_t*)u"DXGI_ERROR_NAME_ALREADY_EXISTS";
			break; 
		case DXGI_ERROR_SDK_COMPONENT_MISSING:
			return (const wchar_t*)u"DXGI_ERROR_SDK_COMPONENT_MISSING"; 
			break; 
		case DXGI_ERROR_NOT_CURRENT:
			return (const wchar_t*)u"DXGI_ERROR_NOT_CURRENT";
			break; 
		case DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY:
			return (const wchar_t*)u"DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY"; 
			break; 
		case DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION:
			return (const wchar_t*)u"DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION"; 
			break; 
		case DXGI_ERROR_NON_COMPOSITED_UI:
			return (const wchar_t*)u"DXGI_ERROR_NON_COMPOSITED_UI";
			break; 
		case DXGI_ERROR_MODE_CHANGE_IN_PROGRESS:
			return (const wchar_t*)u"DXGI_ERROR_MODE_CHANGE_IN_PROGRESS";
			break; 
		case DXGI_ERROR_CACHE_CORRUPT:
			return (const wchar_t*)u"DXGI_ERROR_CACHE_CORRUPT";
			break;
		case DXGI_ERROR_CACHE_FULL:
			return (const wchar_t*)u"DXGI_ERROR_CACHE_FULL";
			break;
		case DXGI_ERROR_CACHE_HASH_COLLISION:
			return (const wchar_t*)u"DXGI_ERROR_CACHE_HASH_COLLISION";
			break;
		case DXGI_ERROR_ALREADY_EXISTS:
			return (const wchar_t*)u"DXGI_ERROR_ALREADY_EXISTS"; 
			break;
		case D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return (const wchar_t*)u"D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break; 
		case D3D10_ERROR_FILE_NOT_FOUND:
			return (const wchar_t*)u"D3D10_ERROR_FILE_NOT_FOUND";
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return (const wchar_t*)u"D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break;
		case D3D11_ERROR_FILE_NOT_FOUND:
			return (const wchar_t*)u"D3D11_ERROR_FILE_NOT_FOUND"; 
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
			return (const wchar_t*)u"D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS"; 
			break; 
		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
			return (const wchar_t*)u"D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
			break;
		case D3D12_ERROR_ADAPTER_NOT_FOUND:
			return (const wchar_t*)u"D3D12_ERROR_ADAPTER_NOT_FOUND";
			break;
		case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
			return (const wchar_t*)u"D3D12_ERROR_DRIVER_VERSION_MISMATCH"; 
			break;
		default:
			return nullptr;
			break;
	}

	return nullptr;
}