// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

struct PointLight
{
	XMFLOAT3 Position;
	float Radius;
	XMFLOAT3 Color;
	float Brightness;
};

struct CameraConstantBuffer
{
	XMMATRIX ViewProjMatrix;
	XMMATRIX InvViewProjMatrix;
	XMFLOAT3 CameraWorldPosition;
	float NearZ, FarZ;
};

struct GBufferOpaquePassConstantBuffer
{
	//XMMATRIX WVPMatrix;
	XMMATRIX WorldMatrix;
	XMFLOAT3X4 VectorTransformMatrix;
};

struct ShadowMapPassConstantBuffer
{
	//XMMATRIX WVPMatrix;
	XMMATRIX WorldMatrix;
};

struct ShadowResolveConstantBuffer
{
	XMMATRIX ReProjMatrices[4];
};

struct DeferredLightingConstantBuffer
{
	XMMATRIX InvViewProjMatrix;
	XMFLOAT3 CameraWorldPosition;
};

struct SkyConstantBuffer
{
	XMMATRIX WVPMatrix;
};

struct SunConstantBuffer
{
	XMMATRIX ViewMatrix;
	XMMATRIX ProjMatrix;
	XMFLOAT3 SunPosition;
};

inline SIZE_T RenderSystem::GetOffsetForResource(D3D12_RESOURCE_DESC& ResourceDesc, D3D12_HEAP_DESC& HeapDesc)
{
	D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);
	SIZE_T ResourceOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + ((HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment == 0) ? 0 : (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment));
	HeapDesc.SizeInBytes = ResourceOffset + ResourceAllocationInfo.SizeInBytes;

	return ResourceOffset;
}

Pointer<Buffer> RenderSystem::CreateBuffer(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC* ResourceDesc, D3D12_RESOURCE_STATES InitialState, const char16_t* DebugName)
{
	Pointer<Buffer> BufferPtr = Pointer<Buffer>::Create();

	SAFE_DX(Device->CreatePlacedResource(Heap, HeapOffset, ResourceDesc, InitialState, nullptr, UUIDOF(BufferPtr->DXBuffer)));
	if (DebugName) SAFE_DX(BufferPtr->DXBuffer->SetName((LPCWSTR)DebugName));

	BufferPtr->BufferState = InitialState;

	return BufferPtr;
}

Pointer<Texture> RenderSystem::CreateTexture(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC* ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue, const char16_t* DebugName)
{
	Pointer<Texture> TexturePtr = Pointer<Texture>::Create();

	SAFE_DX(Device->CreatePlacedResource(Heap, HeapOffset, ResourceDesc, InitialState, OptimizedClearValue, UUIDOF(TexturePtr->DXTexture)));
	if (DebugName) SAFE_DX(TexturePtr->DXTexture->SetName((LPCWSTR)DebugName));

	TexturePtr->SubResourcesCount = ResourceDesc->MipLevels * (ResourceDesc->Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D) ? ResourceDesc->DepthOrArraySize : 1;

	TexturePtr->TextureSubResourceStates = (D3D12_RESOURCE_STATES*)SystemMemoryAllocator::AllocateMemory(sizeof(D3D12_RESOURCE_STATES) * TexturePtr->SubResourcesCount);

	for (UINT i = 0; i < TexturePtr->SubResourcesCount; i++)
	{
		TexturePtr->TextureSubResourceStates[i] = InitialState;
	}

	return TexturePtr;
}

void RenderSystem::InitSystem()
{
	UINT FactoryCreationFlags = 0;

#ifdef _DEBUG
	COMRCPtr<ID3D12Debug3> Debug;
	SAFE_DX(D3D12GetDebugInterface(UUIDOF(Debug)));
	Debug->EnableDebugLayer();
	Debug->SetEnableGPUBasedValidation(TRUE);
	FactoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	COMRCPtr<IDXGIFactory7> Factory;

	SAFE_DX(CreateDXGIFactory2(FactoryCreationFlags, UUIDOF(Factory)));

	COMRCPtr<IDXGIAdapter> Adapter;

	SAFE_DX(Factory->EnumAdapters(0, &Adapter));

	COMRCPtr<IDXGIOutput> Monitor;

	SAFE_DX(Adapter->EnumOutputs(0, &Monitor));

	UINT DisplayModesCount;
	SAFE_DX(Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, nullptr));
	DXGI_MODE_DESC *DisplayModes = new DXGI_MODE_DESC[(size_t)DisplayModesCount];
	SAFE_DX(Monitor->GetDisplayModeList(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, 0, &DisplayModesCount, DisplayModes));

	/*ResolutionWidth = DisplayModes[(size_t)DisplayModesCount - 1].Width;
	ResolutionHeight = DisplayModes[(size_t)DisplayModesCount - 1].Height;*/

	/*ResolutionWidth = 1280;
	ResolutionHeight = 720;*/

#if WITH_EDITOR
	if (Application::IsEditor())
	{
		EditorViewportWidth = Application::EditorViewportWidth;
		EditorViewportHeight = Application::EditorViewportHeight;

		ResolutionWidth = EditorViewportWidth;
		ResolutionHeight = EditorViewportHeight;
	}
	else
#endif
	{
		ResolutionWidth = 1280;
		ResolutionHeight = 720;
	}

	SAFE_DX(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, UUIDOF(Device)));

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask = 0;
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;
	SAFE_DX(Device->CreateCommandQueue(&CommandQueueDesc, UUIDOF(GraphicsCommandQueue)));
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
	SAFE_DX(Device->CreateCommandQueue(&CommandQueueDesc, UUIDOF(ComputeCommandQueue)));
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
	SAFE_DX(Device->CreateCommandQueue(&CommandQueueDesc, UUIDOF(CopyCommandQueue)));
	SAFE_DX(GraphicsCommandQueue->SetName((const wchar_t*)u"Graphics Command Queue"));
	SAFE_DX(ComputeCommandQueue->SetName((const wchar_t*)u"Compute Command Queue"));
	SAFE_DX(CopyCommandQueue->SetName((const wchar_t*)u"Copy Command Queue"));

	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, UUIDOF(GraphicsCommandAllocators[0])));
	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, UUIDOF(GraphicsCommandAllocators[1])));
	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE, UUIDOF(ComputeCommandAllocators[0])));
	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE, UUIDOF(ComputeCommandAllocators[1])));
	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, UUIDOF(CopyCommandAllocator)));
	SAFE_DX(GraphicsCommandAllocators[0]->SetName((const wchar_t*)u"Graphics Command Allocator 0"));
	SAFE_DX(GraphicsCommandAllocators[1]->SetName((const wchar_t*)u"Graphics Command Allocator 1"));
	SAFE_DX(ComputeCommandAllocators[0]->SetName((const wchar_t*)u"Compute Command Allocator 0"));
	SAFE_DX(ComputeCommandAllocators[1]->SetName((const wchar_t*)u"Compute Command Allocator 1"));
	SAFE_DX(CopyCommandAllocator->SetName((const wchar_t*)u"Copy Command Allocator"));

	SAFE_DX(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, GraphicsCommandAllocators[0], nullptr, UUIDOF(GraphicsCommandList)));
	SAFE_DX(GraphicsCommandList->SetName((const wchar_t*)u"Graphics Command List"));
	SAFE_DX(GraphicsCommandList->Close());
	SAFE_DX(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE, ComputeCommandAllocators[0], nullptr, UUIDOF(ComputeCommandList)));
	SAFE_DX(ComputeCommandList->SetName((const wchar_t*)u"Compute Command List"));
	SAFE_DX(ComputeCommandList->Close());
	SAFE_DX(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY, CopyCommandAllocator, nullptr, UUIDOF(CopyCommandList)));
	SAFE_DX(CopyCommandList->SetName((const wchar_t*)u"Copy Command List"));
	SAFE_DX(CopyCommandList->Close());

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
	SwapChainFullScreenDesc.RefreshRate.Numerator = DisplayModes[(size_t)DisplayModesCount - 1].RefreshRate.Numerator;
	SwapChainFullScreenDesc.RefreshRate.Denominator = DisplayModes[(size_t)DisplayModesCount - 1].RefreshRate.Denominator;
	SwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChainFullScreenDesc.Windowed = TRUE;

	HWND RenderTargetHandle =
#if WITH_EDITOR
		Application::IsEditor()
		?
		Application::GetLevelRenderCanvasHandle() :
#endif
		Application::GetMainWindowHandle();

	COMRCPtr<IDXGISwapChain1> SwapChain1;
	SAFE_DX(Factory->CreateSwapChainForHwnd(GraphicsCommandQueue, RenderTargetHandle, &SwapChainDesc, &SwapChainFullScreenDesc, nullptr, &SwapChain1));
	SAFE_DX(SwapChain1->QueryInterface<IDXGISwapChain4>(&SwapChain));

	SAFE_DX(Factory->MakeWindowAssociation(RenderTargetHandle, DXGI_MWA_NO_ALT_ENTER));

	delete[] DisplayModes;

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CurrentFrameIndex = 0;

	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(FrameSyncFences[0])));
	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(FrameSyncFences[1])));
	SAFE_DX(FrameSyncFences[0]->SetName((const wchar_t*)u"Frame Sync Fence 0"));
	SAFE_DX(FrameSyncFences[1]->SetName((const wchar_t*)u"Frame Sync Fence 1"));

	FrameSyncEvent = CreateEvent(NULL, FALSE, FALSE, (const wchar_t*)u"FrameSyncEvent");

	SAFE_DX(Device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(CopySyncFence)));
	SAFE_DX(CopySyncFence->SetName((const wchar_t*)u"Copy Sync Fence"));

	CopySyncEvent = CreateEvent(NULL, FALSE, FALSE, (const wchar_t*)u"CopySyncEvent");

	RTDescriptorHeap = Pointer<DescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 43, Device, u"Render Targets Descriptor Heap");
	DSDescriptorHeap = Pointer<DescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5, Device, u"Depth-Stencils Descriptor Heap");
	CBSRUADescriptorHeap = Pointer<DescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 63, Device, u"Constant Buffer/Shader Resource/Unordered Access Views Descriptor Heap");
	SamplersDescriptorHeap = Pointer<DescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 4, Device, u"Samplers Descriptor Heap");

	ConstantBufferDescriptorHeap = Pointer<DescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 120000, Device, u"Constant Buffers Descriptor Heap");
	TexturesDescriptorHeap = Pointer<DescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8000, Device, u"Textures Descriptor Heap");

	FrameResourcesDescriptorHeaps[0] = Pointer<FrameDescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 500000, Device, u"Frame Resources Descriptor Heap 0");
	FrameResourcesDescriptorHeaps[1] = Pointer<FrameDescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 500000, Device, u"Frame Resources Descriptor Heap 0");

	FrameSamplersDescriptorHeaps[0] = Pointer<FrameDescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2000, Device, u"Frame Samplers Descriptor Heap 1");
	FrameSamplersDescriptorHeaps[1] = Pointer<FrameDescriptorHeap>::Create(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2000, Device, u"Frame Samplers Descriptor Heap 1");

	D3D12_DESCRIPTOR_RANGE DescriptorRanges[4];
	DescriptorRanges[0].BaseShaderRegister = 0;
	DescriptorRanges[0].NumDescriptors = 4;
	DescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescriptorRanges[0].RegisterSpace = 0;
	DescriptorRanges[1].BaseShaderRegister = 0;
	DescriptorRanges[1].NumDescriptors = 8;
	DescriptorRanges[1].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescriptorRanges[1].RegisterSpace = 0;
	DescriptorRanges[2].BaseShaderRegister = 0;
	DescriptorRanges[2].NumDescriptors = 1;
	DescriptorRanges[2].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	DescriptorRanges[2].RegisterSpace = 0;
	DescriptorRanges[3].BaseShaderRegister = 0;
	DescriptorRanges[3].NumDescriptors = 1;
	DescriptorRanges[3].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	DescriptorRanges[3].RegisterSpace = 0;

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

	COMRCPtr<ID3DBlob> RootSignatureBlob;

	SAFE_DX(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSignatureBlob, nullptr));

	SAFE_DX(Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), UUIDOF(GraphicsRootSignature)));
	SAFE_DX(GraphicsRootSignature->SetName((const wchar_t*)u"Graphics Root Signature"));

	RootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[0].DescriptorTable.pDescriptorRanges = &DescriptorRanges[0];
	RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	RootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[1].DescriptorTable.pDescriptorRanges = &DescriptorRanges[1];
	RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	RootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[2].DescriptorTable.pDescriptorRanges = &DescriptorRanges[2];
	RootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	RootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	RootParameters[3].DescriptorTable.pDescriptorRanges = &DescriptorRanges[3];
	RootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;

	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	RootSignatureDesc.NumParameters = 4;
	RootSignatureDesc.NumStaticSamplers = 0;
	RootSignatureDesc.pParameters = RootParameters;
	RootSignatureDesc.pStaticSamplers = nullptr;

	SAFE_DX(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSignatureBlob, nullptr));

	SAFE_DX(Device->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), UUIDOF(ComputeRootSignature)));
	SAFE_DX(ComputeRootSignature->SetName((const wchar_t*)u"Compute Root Signature"));

	{
		SAFE_DX(SwapChain->GetBuffer(0, UUIDOF(BackBufferTextures[0])));
		SAFE_DX(SwapChain->GetBuffer(1, UUIDOF(BackBufferTextures[1])));
		SAFE_DX(BackBufferTextures[0]->SetName((const wchar_t*)u"Back Buffer Texture 0"));
		SAFE_DX(BackBufferTextures[1]->SetName((const wchar_t*)u"Back Buffer Texture 1"));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		BackBufferTexturesRTVs[0] = RTDescriptorHeap->AllocateDescriptor();
		BackBufferTexturesRTVs[1] = RTDescriptorHeap->AllocateDescriptor();

		Device->CreateRenderTargetView(BackBufferTextures[0], &RTVDesc, BackBufferTexturesRTVs[0]);
		Device->CreateRenderTargetView(BackBufferTextures[1], &RTVDesc, BackBufferTexturesRTVs[1]);
	}

	// ===============================================================================================================

	void *FullScreenQuadVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.FullScreenQuad");
	size_t FullScreenQuadVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.FullScreenQuad");

	// ===============================================================================================================

	{
		D3D12_SAMPLER_DESC SamplerDesc;
		ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
		SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
		SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)0;
		SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_ANISOTROPIC;
		SamplerDesc.MaxAnisotropy = 16;
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		SamplerDesc.MinLOD = 0;
		SamplerDesc.MipLODBias = 0.0f;

		TextureSampler = SamplersDescriptorHeap->AllocateDescriptor();

		Device->CreateSampler(&SamplerDesc, TextureSampler);

		ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
		SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
		SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
		SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		SamplerDesc.MaxAnisotropy = 1;
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		SamplerDesc.MinLOD = 0;
		SamplerDesc.MipLODBias = 0.0f;

		ShadowMapSampler = SamplersDescriptorHeap->AllocateDescriptor();

		Device->CreateSampler(&SamplerDesc, ShadowMapSampler);

		ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
		SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
		SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)0;
		SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		SamplerDesc.MaxAnisotropy = 1;
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		SamplerDesc.MinLOD = 0;
		SamplerDesc.MipLODBias = 0.0f;

		BiLinearSampler = SamplersDescriptorHeap->AllocateDescriptor();

		Device->CreateSampler(&SamplerDesc, BiLinearSampler);

		ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
		SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
		SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)0;
		SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
		SamplerDesc.MaxAnisotropy = 1;
		SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		SamplerDesc.MinLOD = 0;
		SamplerDesc.MipLODBias = 0.0f;

		MinSampler = SamplersDescriptorHeap->AllocateDescriptor();

		Device->CreateSampler(&SamplerDesc, MinSampler);
	}

	// ===============================================================================================================

	{
		D3D12_HEAP_DESC HeapDesc;
		ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = BUFFER_MEMORY_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex])));

		ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = TEXTURE_MEMORY_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex])));

		ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = UPLOAD_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(UploadHeap)));

		D3D12_RESOURCE_DESC ResourceDesc;
		ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
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

		SAFE_DX(Device->CreatePlacedResource(UploadHeap, 0, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(UploadBuffer)));
		SAFE_DX(UploadBuffer->SetName((const wchar_t*)u"Upload Buffer"));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ConstantBufferResourceDesc;
		ZeroMemory(&ConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ConstantBufferResourceDesc.Alignment = 0;
		ConstantBufferResourceDesc.DepthOrArraySize = 1;
		ConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		ConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		ConstantBufferResourceDesc.Height = 1;
		ConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ConstantBufferResourceDesc.MipLevels = 1;
		ConstantBufferResourceDesc.SampleDesc.Count = 1;
		ConstantBufferResourceDesc.SampleDesc.Quality = 0;
		ConstantBufferResourceDesc.Width = 256;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T GPUCameraConstantBufferOffset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T GPURenderTargetConstantBufferOffset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory0)));
		SAFE_DX(GPUMemory0->SetName((const wchar_t*)u"Camera Constants Data GPU Heap"));

		GPUCameraConstantBuffer = CreateBuffer(GPUMemory0, GPUCameraConstantBufferOffset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Camera Constant Buffer");
		GPURenderTargetConstantBuffer = CreateBuffer(GPUMemory0, GPURenderTargetConstantBufferOffset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Render Target Constant Buffer");

		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CPUCameraConstantBuffer0Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUCameraConstantBuffer1Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPURenderTargetConstantBuffer0Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPURenderTargetConstantBuffer1Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory0)));
		SAFE_DX(CPUMemory0->SetName((const wchar_t*)u"Camera Constants Data CPU Heap"));

		CPUCameraConstantBuffers[0] = CreateBuffer(CPUMemory0, CPUCameraConstantBuffer0Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Camera Constant Buffers 0");
		CPUCameraConstantBuffers[1] = CreateBuffer(CPUMemory0, CPUCameraConstantBuffer1Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Camera Constant Buffers 1");
		CPURenderTargetConstantBuffers[0] = CreateBuffer(CPUMemory0, CPURenderTargetConstantBuffer0Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Render Target Constant Buffers 0");
		CPURenderTargetConstantBuffers[1] = CreateBuffer(CPUMemory0, CPURenderTargetConstantBuffer1Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Render Target Constant Buffers 1");

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUCameraConstantBuffer->DXBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		CameraConstantBufferCBV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateConstantBufferView(&CBVDesc, CameraConstantBufferCBV);

		CBVDesc.BufferLocation = GPURenderTargetConstantBuffer->DXBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		RenderTargetConstantBufferCBV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateConstantBufferView(&CBVDesc, RenderTargetConstantBufferCBV);
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC GBufferTexture0ResourceDesc;
		ZeroMemory(&GBufferTexture0ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		GBufferTexture0ResourceDesc.Alignment = 0;
		GBufferTexture0ResourceDesc.DepthOrArraySize = 1;
		GBufferTexture0ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		GBufferTexture0ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		GBufferTexture0ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		GBufferTexture0ResourceDesc.Height = ResolutionHeight;
		GBufferTexture0ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		GBufferTexture0ResourceDesc.MipLevels = 1;
		GBufferTexture0ResourceDesc.SampleDesc.Count = 8;
		GBufferTexture0ResourceDesc.SampleDesc.Quality = 0;
		GBufferTexture0ResourceDesc.Width = ResolutionWidth;

		D3D12_RESOURCE_DESC GBufferTexture1ResourceDesc;
		ZeroMemory(&GBufferTexture1ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		GBufferTexture1ResourceDesc.Alignment = 0;
		GBufferTexture1ResourceDesc.DepthOrArraySize = 1;
		GBufferTexture1ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		GBufferTexture1ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		GBufferTexture1ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		GBufferTexture1ResourceDesc.Height = ResolutionHeight;
		GBufferTexture1ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		GBufferTexture1ResourceDesc.MipLevels = 1;
		GBufferTexture1ResourceDesc.SampleDesc.Count = 8;
		GBufferTexture1ResourceDesc.SampleDesc.Quality = 0;
		GBufferTexture1ResourceDesc.Width = ResolutionWidth;

		D3D12_RESOURCE_DESC GBufferTexture2ResourceDesc;
		ZeroMemory(&GBufferTexture2ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		GBufferTexture2ResourceDesc.Alignment = 0;
		GBufferTexture2ResourceDesc.DepthOrArraySize = 1;
		GBufferTexture2ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		GBufferTexture2ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		GBufferTexture2ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GBufferTexture2ResourceDesc.Height = ResolutionHeight;
		GBufferTexture2ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		GBufferTexture2ResourceDesc.MipLevels = 1;
		GBufferTexture2ResourceDesc.SampleDesc.Count = 8;
		GBufferTexture2ResourceDesc.SampleDesc.Quality = 0;
		GBufferTexture2ResourceDesc.Width = ResolutionWidth;

		D3D12_RESOURCE_DESC DepthBufferTextureResourceDesc;
		ZeroMemory(&DepthBufferTextureResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		DepthBufferTextureResourceDesc.Alignment = 0;
		DepthBufferTextureResourceDesc.DepthOrArraySize = 1;
		DepthBufferTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		DepthBufferTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		DepthBufferTextureResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		DepthBufferTextureResourceDesc.Height = ResolutionHeight;
		DepthBufferTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		DepthBufferTextureResourceDesc.MipLevels = 1;
		DepthBufferTextureResourceDesc.SampleDesc.Count = 8;
		DepthBufferTextureResourceDesc.SampleDesc.Quality = 0;
		DepthBufferTextureResourceDesc.Width = ResolutionWidth;

		D3D12_RESOURCE_DESC GBufferOpaquePassConstantBufferResourceDesc;
		ZeroMemory(&GBufferOpaquePassConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		GBufferOpaquePassConstantBufferResourceDesc.Alignment = 0;
		GBufferOpaquePassConstantBufferResourceDesc.DepthOrArraySize = 1;
		GBufferOpaquePassConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		GBufferOpaquePassConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		GBufferOpaquePassConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		GBufferOpaquePassConstantBufferResourceDesc.Height = 1;
		GBufferOpaquePassConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		GBufferOpaquePassConstantBufferResourceDesc.MipLevels = 1;
		GBufferOpaquePassConstantBufferResourceDesc.SampleDesc.Count = 1;
		GBufferOpaquePassConstantBufferResourceDesc.SampleDesc.Quality = 0;
		GBufferOpaquePassConstantBufferResourceDesc.Width = 256 * 20000;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T GBufferTexture0Offset = GetOffsetForResource(GBufferTexture0ResourceDesc, HeapDesc);
		SIZE_T GBufferTexture1Offset = GetOffsetForResource(GBufferTexture1ResourceDesc, HeapDesc);
		SIZE_T GBufferTexture2Offset = GetOffsetForResource(GBufferTexture2ResourceDesc, HeapDesc);
		SIZE_T DepthBufferTextureOffset = GetOffsetForResource(DepthBufferTextureResourceDesc, HeapDesc);
		SIZE_T GPUConstantBufferOffset = GetOffsetForResource(GBufferOpaquePassConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory1)));
		SAFE_DX(GPUMemory1->SetName((const wchar_t*)u"G-Buffer Opaque Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		GBufferTextures[0] = CreateTexture(GPUMemory1, GBufferTexture0Offset, &GBufferTexture0ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"G-Buffer Texture 0");

		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;

		GBufferTextures[1] = CreateTexture(GPUMemory1, GBufferTexture1Offset, &GBufferTexture1ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"G-Buffer Texture 1");

		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

		GBufferTextures[2] = CreateTexture(GPUMemory1, GBufferTexture2Offset, &GBufferTexture2ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"G-Buffer Texture 2");

		ClearValue.DepthStencil.Depth = 0.0f;
		ClearValue.DepthStencil.Stencil = 0;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		DepthBufferTexture = CreateTexture(GPUMemory1, DepthBufferTextureOffset, &DepthBufferTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, u"Depth Buffer Texture");

		GPUGBufferOpaquePassObjectsConstantBuffer = CreateBuffer(GPUMemory1, GPUConstantBufferOffset, &GBufferOpaquePassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU G-Buffer Opaque Pass Objects Constant Buffer");

		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CPUConstantBuffer0Offset = GetOffsetForResource(GBufferOpaquePassConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUConstantBuffer1Offset = GetOffsetForResource(GBufferOpaquePassConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory1)));
		SAFE_DX(CPUMemory1->SetName((const wchar_t*)u"G-Buffer Opaque Pass Data CPU Heap"));

		CPUGBufferOpaquePassObjectsConstantBuffers[0] = CreateBuffer(CPUMemory1, CPUConstantBuffer0Offset, &GBufferOpaquePassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU G-Buffer Opaque Pass Objects Constant Buffers 0");
		CPUGBufferOpaquePassObjectsConstantBuffers[1] = CreateBuffer(CPUMemory1, CPUConstantBuffer1Offset, &GBufferOpaquePassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU G-Buffer Opaque Pass Objects Constant Buffers 1");

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

		GBufferTexturesRTVs[0] = RTDescriptorHeap->AllocateDescriptor();
		GBufferTexturesRTVs[1] = RTDescriptorHeap->AllocateDescriptor();
		GBufferTexturesRTVs[2] = RTDescriptorHeap->AllocateDescriptor();

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		Device->CreateRenderTargetView(GBufferTextures[0]->DXTexture, &RTVDesc, GBufferTexturesRTVs[0]);

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		Device->CreateRenderTargetView(GBufferTextures[1]->DXTexture, &RTVDesc, GBufferTexturesRTVs[1]);

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		Device->CreateRenderTargetView(GBufferTextures[2]->DXTexture, &RTVDesc, GBufferTexturesRTVs[2]);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

		GBufferTexturesSRVs[0] = CBSRUADescriptorHeap->AllocateDescriptor();
		GBufferTexturesSRVs[1] = CBSRUADescriptorHeap->AllocateDescriptor();
		GBufferTexturesSRVs[2] = CBSRUADescriptorHeap->AllocateDescriptor();

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		Device->CreateShaderResourceView(GBufferTextures[0]->DXTexture, &SRVDesc, GBufferTexturesSRVs[0]);

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		Device->CreateShaderResourceView(GBufferTextures[1]->DXTexture, &SRVDesc, GBufferTexturesSRVs[1]);

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		Device->CreateShaderResourceView(GBufferTextures[2]->DXTexture, &SRVDesc, GBufferTexturesSRVs[2]);

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
		DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DMS;

		DepthBufferTextureDSV = DSDescriptorHeap->AllocateDescriptor();

		Device->CreateDepthStencilView(DepthBufferTexture->DXTexture, &DSVDesc, DepthBufferTextureDSV);

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

		DepthBufferTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(DepthBufferTexture->DXTexture, &SRVDesc, DepthBufferTextureSRV);

		for (int i = 0; i < 20000; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = GPUGBufferOpaquePassObjectsConstantBuffer->DXBuffer->GetGPUVirtualAddress() + i * 256;
			CBVDesc.SizeInBytes = 256;

			GBufferOpaquePassObjectsConstantBufferCBVs[i] = ConstantBufferDescriptorHeap->AllocateDescriptor();

			Device->CreateConstantBufferView(&CBVDesc, GBufferOpaquePassObjectsConstantBufferCBVs[i]);
		}
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ResourceDesc;
		ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ResourceDesc.Alignment = 0;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		ResourceDesc.Height = ResolutionHeight;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Width = ResolutionWidth;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T ResolvedDepthBufferTextureOffset = GetOffsetForResource(ResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory2)));
		SAFE_DX(GPUMemory2->SetName((const wchar_t*)u"Depth Resolve Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.DepthStencil.Depth = 1.0f;
		ClearValue.DepthStencil.Stencil = 0;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		ResolvedDepthBufferTexture = CreateTexture(GPUMemory2, ResolvedDepthBufferTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, u"Resolved Depth Buffer Texture");

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		ResolvedDepthBufferTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(ResolvedDepthBufferTexture->DXTexture, &SRVDesc, ResolvedDepthBufferTextureSRV);
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ResourceDesc;
		ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ResourceDesc.Alignment = 0;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		ResourceDesc.Height = 144;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Width = 256;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T OcclusionBufferTextureOffset = GetOffsetForResource(ResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory3)));
		SAFE_DX(GPUMemory3->SetName((const wchar_t*)u"Occlusion Buffer Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

		OcclusionBufferTexture = CreateTexture(GPUMemory3, OcclusionBufferTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Occlusion Buffer Texture");

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

		UINT NumRows;
		UINT64 RowSizeInBytes, TotalBytes;

		Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

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

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T OcclusionBufferReadbackBuffer0Offset = GetOffsetForResource(ResourceDesc, HeapDesc);
		SIZE_T OcclusionBufferReadbackBuffer1Offset = GetOffsetForResource(ResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory3)));
		SAFE_DX(CPUMemory3->SetName((const wchar_t*)u"Occlsuon Buffer Pass Data CPU Heap"));

		OcclusionBufferTextureReadback[0] = CreateBuffer(CPUMemory3, OcclusionBufferReadbackBuffer0Offset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, u"Occlusion Buffer Texture Readback 0");
		OcclusionBufferTextureReadback[1] = CreateBuffer(CPUMemory3, OcclusionBufferReadbackBuffer1Offset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, u"Occlusion Buffer Texture Readback 1");

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		OcclusionBufferTextureRTV = RTDescriptorHeap->AllocateDescriptor();

		Device->CreateRenderTargetView(OcclusionBufferTexture->DXTexture, &RTVDesc, OcclusionBufferTextureRTV);

		void *OcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.OcclusionBuffer");
		SIZE_T OcclusionBufferPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.OcclusionBuffer");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = OcclusionBufferPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = OcclusionBufferPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(OcclusionBufferPipelineState)));
		SAFE_DX(OcclusionBufferPipelineState->SetName((const wchar_t*)u"Occlusion Buffer Pipeline State"));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ShadowMapTextureResourceDesc;
		ZeroMemory(&ShadowMapTextureResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ShadowMapTextureResourceDesc.Alignment = 0;
		ShadowMapTextureResourceDesc.DepthOrArraySize = 1;
		ShadowMapTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ShadowMapTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		ShadowMapTextureResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		ShadowMapTextureResourceDesc.Height = 2048;
		ShadowMapTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ShadowMapTextureResourceDesc.MipLevels = 1;
		ShadowMapTextureResourceDesc.SampleDesc.Count = 1;
		ShadowMapTextureResourceDesc.SampleDesc.Quality = 0;
		ShadowMapTextureResourceDesc.Width = 2048;

		D3D12_RESOURCE_DESC ShadowMapPassConstantBufferResourceDesc;
		ZeroMemory(&ShadowMapPassConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ShadowMapPassConstantBufferResourceDesc.Alignment = 0;
		ShadowMapPassConstantBufferResourceDesc.DepthOrArraySize = 1;
		ShadowMapPassConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		ShadowMapPassConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ShadowMapPassConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		ShadowMapPassConstantBufferResourceDesc.Height = 1;
		ShadowMapPassConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ShadowMapPassConstantBufferResourceDesc.MipLevels = 1;
		ShadowMapPassConstantBufferResourceDesc.SampleDesc.Count = 1;
		ShadowMapPassConstantBufferResourceDesc.SampleDesc.Quality = 0;
		ShadowMapPassConstantBufferResourceDesc.Width = 256 * 20000;

		D3D12_RESOURCE_DESC ShadowMapCameraConstantBufferResourceDesc;
		ZeroMemory(&ShadowMapCameraConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ShadowMapCameraConstantBufferResourceDesc.Alignment = 0;
		ShadowMapCameraConstantBufferResourceDesc.DepthOrArraySize = 1;
		ShadowMapCameraConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		ShadowMapCameraConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ShadowMapCameraConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		ShadowMapCameraConstantBufferResourceDesc.Height = 1;
		ShadowMapCameraConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ShadowMapCameraConstantBufferResourceDesc.MipLevels = 1;
		ShadowMapCameraConstantBufferResourceDesc.SampleDesc.Count = 1;
		ShadowMapCameraConstantBufferResourceDesc.SampleDesc.Quality = 0;
		ShadowMapCameraConstantBufferResourceDesc.Width = 256 * 4;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CascadedShadowMapTexture0Offset = GetOffsetForResource(ShadowMapTextureResourceDesc, HeapDesc);
		SIZE_T CascadedShadowMapTexture1Offset = GetOffsetForResource(ShadowMapTextureResourceDesc, HeapDesc);
		SIZE_T CascadedShadowMapTexture2Offset = GetOffsetForResource(ShadowMapTextureResourceDesc, HeapDesc);
		SIZE_T CascadedShadowMapTexture3Offset = GetOffsetForResource(ShadowMapTextureResourceDesc, HeapDesc);

		SIZE_T ConstantBuffer0Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer1Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer2Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer3Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T CameraConstantBufferOffset = GetOffsetForResource(ShadowMapCameraConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory4)));
		SAFE_DX(GPUMemory4->SetName((const wchar_t*)u"Shadow Map Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.DepthStencil.Depth = 1.0f;
		ClearValue.DepthStencil.Stencil = 0;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

		CascadedShadowMapTextures[0] = CreateTexture(GPUMemory4, CascadedShadowMapTexture0Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, u"Cascaded Shadow Map Textures [0]");
		CascadedShadowMapTextures[1] = CreateTexture(GPUMemory4, CascadedShadowMapTexture1Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, u"Cascaded Shadow Map Textures [1]");
		CascadedShadowMapTextures[2] = CreateTexture(GPUMemory4, CascadedShadowMapTexture2Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, u"Cascaded Shadow Map Textures [2]");
		CascadedShadowMapTextures[3] = CreateTexture(GPUMemory4, CascadedShadowMapTexture3Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, u"Cascaded Shadow Map Textures [3]");

		GPUShadowMapPassObjectsConstantBuffers[0] = CreateBuffer(GPUMemory4, ConstantBuffer0Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Shadow Map Pass Objects Constant Buffer [0]");
		GPUShadowMapPassObjectsConstantBuffers[1] = CreateBuffer(GPUMemory4, ConstantBuffer1Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Shadow Map Pass Objects Constant Buffer [1]");
		GPUShadowMapPassObjectsConstantBuffers[2] = CreateBuffer(GPUMemory4, ConstantBuffer2Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Shadow Map Pass Objects Constant Buffer [2]");
		GPUShadowMapPassObjectsConstantBuffers[3] = CreateBuffer(GPUMemory4, ConstantBuffer3Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Shadow Map Pass Objects Constant Buffer [3]");

		GPUShadowMapCameraConstantBuffer = CreateBuffer(GPUMemory4, CameraConstantBufferOffset, &ShadowMapCameraConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Shadow Map Camera Constant Buffer");
		
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T ConstantBuffer00Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer01Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer10Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer11Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer20Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer21Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer30Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T ConstantBuffer31Offset = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUCameraConstantBufferOffset0 = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUCameraConstantBufferOffset1 = GetOffsetForResource(ShadowMapPassConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory4)));
		SAFE_DX(CPUMemory4->SetName((const wchar_t*)u"Shadow Map Pass Data CPU Heap"));

		CPUShadowMapPassObjectsConstantBuffers[0][0] = CreateBuffer(CPUMemory4, ConstantBuffer00Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [0] 0");
		CPUShadowMapPassObjectsConstantBuffers[1][0] = CreateBuffer(CPUMemory4, ConstantBuffer10Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [1] 0");
		CPUShadowMapPassObjectsConstantBuffers[2][0] = CreateBuffer(CPUMemory4, ConstantBuffer20Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [2] 0");
		CPUShadowMapPassObjectsConstantBuffers[3][0] = CreateBuffer(CPUMemory4, ConstantBuffer30Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [3] 0");
		CPUShadowMapPassObjectsConstantBuffers[0][1] = CreateBuffer(CPUMemory4, ConstantBuffer01Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [0] 1");
		CPUShadowMapPassObjectsConstantBuffers[1][1] = CreateBuffer(CPUMemory4, ConstantBuffer11Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [1] 1");
		CPUShadowMapPassObjectsConstantBuffers[2][1] = CreateBuffer(CPUMemory4, ConstantBuffer21Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [2] 1");
		CPUShadowMapPassObjectsConstantBuffers[3][1] = CreateBuffer(CPUMemory4, ConstantBuffer31Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Pass Objects Constant Buffer [3] 1");

		CPUShadowMapCameraConstantBuffers[0] = CreateBuffer(CPUMemory4, CPUCameraConstantBufferOffset0, &ShadowMapCameraConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Camera Constant Buffer 0");
		CPUShadowMapCameraConstantBuffers[1] = CreateBuffer(CPUMemory4, CPUCameraConstantBufferOffset1, &ShadowMapCameraConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Map Camera Constant Buffer 1");

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
		DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		DSVDesc.Texture2D.MipSlice = 0;
		DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		CascadedShadowMapTexturesDSVs[0] = DSDescriptorHeap->AllocateDescriptor();
		CascadedShadowMapTexturesDSVs[1] = DSDescriptorHeap->AllocateDescriptor();
		CascadedShadowMapTexturesDSVs[2] = DSDescriptorHeap->AllocateDescriptor();
		CascadedShadowMapTexturesDSVs[3] = DSDescriptorHeap->AllocateDescriptor();

		Device->CreateDepthStencilView(CascadedShadowMapTextures[0]->DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[0]);
		Device->CreateDepthStencilView(CascadedShadowMapTextures[1]->DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[1]);
		Device->CreateDepthStencilView(CascadedShadowMapTextures[2]->DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[2]);
		Device->CreateDepthStencilView(CascadedShadowMapTextures[3]->DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[3]);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		CascadedShadowMapTexturesSRVs[0] = CBSRUADescriptorHeap->AllocateDescriptor();
		CascadedShadowMapTexturesSRVs[1] = CBSRUADescriptorHeap->AllocateDescriptor();
		CascadedShadowMapTexturesSRVs[2] = CBSRUADescriptorHeap->AllocateDescriptor();
		CascadedShadowMapTexturesSRVs[3] = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(CascadedShadowMapTextures[0]->DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[0]);
		Device->CreateShaderResourceView(CascadedShadowMapTextures[1]->DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[1]);
		Device->CreateShaderResourceView(CascadedShadowMapTextures[2]->DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[2]);
		Device->CreateShaderResourceView(CascadedShadowMapTextures[3]->DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[3]);

		for (int j = 0; j < 4; j++)
		{
			ShadowMapCameraConstantBufferCBVs[j] = CBSRUADescriptorHeap->AllocateDescriptor();

			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = GPUShadowMapCameraConstantBuffer->DXBuffer->GetGPUVirtualAddress() + j * 256;
			CBVDesc.SizeInBytes = 256;

			Device->CreateConstantBufferView(&CBVDesc, ShadowMapCameraConstantBufferCBVs[j]);

			for (int i = 0; i < 20000; i++)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
				CBVDesc.BufferLocation = GPUShadowMapPassObjectsConstantBuffers[j]->DXBuffer->GetGPUVirtualAddress() + i * 256;
				CBVDesc.SizeInBytes = 256;

				ShadowMapPassObjectsConstantBufferCBVs[j][i] = ConstantBufferDescriptorHeap->AllocateDescriptor();
				
				Device->CreateConstantBufferView(&CBVDesc, ShadowMapPassObjectsConstantBufferCBVs[j][i]);
			}
		}
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ShadowMaskTextureResourceDesc;
		ZeroMemory(&ShadowMaskTextureResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ShadowMaskTextureResourceDesc.Alignment = 0;
		ShadowMaskTextureResourceDesc.DepthOrArraySize = 1;
		ShadowMaskTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ShadowMaskTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		ShadowMaskTextureResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		ShadowMaskTextureResourceDesc.Height = ResolutionHeight;
		ShadowMaskTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ShadowMaskTextureResourceDesc.MipLevels = 1;
		ShadowMaskTextureResourceDesc.SampleDesc.Count = 1;
		ShadowMaskTextureResourceDesc.SampleDesc.Quality = 0;
		ShadowMaskTextureResourceDesc.Width = ResolutionWidth;

		D3D12_RESOURCE_DESC ShadowResolveConstantBufferResourceDesc;
		ZeroMemory(&ShadowResolveConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ShadowResolveConstantBufferResourceDesc.Alignment = 0;
		ShadowResolveConstantBufferResourceDesc.DepthOrArraySize = 1;
		ShadowResolveConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		ShadowResolveConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ShadowResolveConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		ShadowResolveConstantBufferResourceDesc.Height = 1;
		ShadowResolveConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ShadowResolveConstantBufferResourceDesc.MipLevels = 1;
		ShadowResolveConstantBufferResourceDesc.SampleDesc.Count = 1;
		ShadowResolveConstantBufferResourceDesc.SampleDesc.Quality = 0;
		ShadowResolveConstantBufferResourceDesc.Width = 256;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		SIZE_T ShadowMaskTextureOffset = GetOffsetForResource(ShadowMaskTextureResourceDesc, HeapDesc);
		SIZE_T GPUConstantBufferOffset = GetOffsetForResource(ShadowResolveConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory5)));
		SAFE_DX(GPUMemory5->SetName((const wchar_t*)u"Shadow Resolve Pass Data GPU Heap"));

		ShadowMaskTexture = CreateTexture(GPUMemory5, ShadowMaskTextureOffset, &ShadowMaskTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Shadow Mask Texture");

		GPUShadowResolveConstantBuffer = CreateBuffer(GPUMemory5, GPUConstantBufferOffset, &ShadowResolveConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Shadow Resolve Constant Buffer");

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CPUConstantBuffer0Offset = GetOffsetForResource(ShadowResolveConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUConstantBuffer1Offset = GetOffsetForResource(ShadowResolveConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory5)));
		SAFE_DX(CPUMemory5->SetName((const wchar_t*)u"Shadow Resolve Pass Data CPU Heap"));

		CPUShadowResolveConstantBuffers[0] = CreateBuffer(CPUMemory5, CPUConstantBuffer0Offset, &ShadowResolveConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Resolve Constant Buffer 0");
		CPUShadowResolveConstantBuffers[1] = CreateBuffer(CPUMemory5, CPUConstantBuffer1Offset, &ShadowResolveConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Shadow Resolve Constant Buffer 1");

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		ShadowMaskTextureRTV = RTDescriptorHeap->AllocateDescriptor();

		Device->CreateRenderTargetView(ShadowMaskTexture->DXTexture, &RTVDesc, ShadowMaskTextureRTV);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		ShadowMaskTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(ShadowMaskTexture->DXTexture, &SRVDesc, ShadowMaskTextureSRV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUShadowResolveConstantBuffer->DXBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		ShadowResolveConstantBufferCBV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateConstantBufferView(&CBVDesc, ShadowResolveConstantBufferCBV);

		void *ShadowResolvePixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.ShadowResolve");
		SIZE_T ShadowResolvePixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.ShadowResolve");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = ShadowResolvePixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = ShadowResolvePixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(ShadowResolvePipelineState)));
		SAFE_DX(ShadowResolvePipelineState->SetName((const wchar_t*)u"Shadow Resolve Pipeline State"));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC HDRSceneColorTextureResourceDesc;
		ZeroMemory(&HDRSceneColorTextureResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		HDRSceneColorTextureResourceDesc.Alignment = 0;
		HDRSceneColorTextureResourceDesc.DepthOrArraySize = 1;
		HDRSceneColorTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		HDRSceneColorTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		HDRSceneColorTextureResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		HDRSceneColorTextureResourceDesc.Height = ResolutionHeight;
		HDRSceneColorTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		HDRSceneColorTextureResourceDesc.MipLevels = 1;
		HDRSceneColorTextureResourceDesc.SampleDesc.Count = 8;
		HDRSceneColorTextureResourceDesc.SampleDesc.Quality = 0;
		HDRSceneColorTextureResourceDesc.Width = ResolutionWidth;

		D3D12_RESOURCE_DESC ConstantBufferResourceDesc;
		ZeroMemory(&ConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ConstantBufferResourceDesc.Alignment = 0;
		ConstantBufferResourceDesc.DepthOrArraySize = 1;
		ConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		ConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		ConstantBufferResourceDesc.Height = 1;
		ConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ConstantBufferResourceDesc.MipLevels = 1;
		ConstantBufferResourceDesc.SampleDesc.Count = 1;
		ConstantBufferResourceDesc.SampleDesc.Quality = 0;
		ConstantBufferResourceDesc.Width = 256;

		D3D12_RESOURCE_DESC LightClustersBufferResourceDesc;
		ZeroMemory(&LightClustersBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		LightClustersBufferResourceDesc.Alignment = 0;
		LightClustersBufferResourceDesc.DepthOrArraySize = 1;
		LightClustersBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		LightClustersBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		LightClustersBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		LightClustersBufferResourceDesc.Height = 1;
		LightClustersBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		LightClustersBufferResourceDesc.MipLevels = 1;
		LightClustersBufferResourceDesc.SampleDesc.Count = 1;
		LightClustersBufferResourceDesc.SampleDesc.Quality = 0;
		LightClustersBufferResourceDesc.Width = sizeof(LightCluster) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;

		D3D12_RESOURCE_DESC LightIndicesBufferResourceDesc;
		ZeroMemory(&LightIndicesBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		LightIndicesBufferResourceDesc.Alignment = 0;
		LightIndicesBufferResourceDesc.DepthOrArraySize = 1;
		LightIndicesBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		LightIndicesBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		LightIndicesBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		LightIndicesBufferResourceDesc.Height = 1;
		LightIndicesBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		LightIndicesBufferResourceDesc.MipLevels = 1;
		LightIndicesBufferResourceDesc.SampleDesc.Count = 1;
		LightIndicesBufferResourceDesc.SampleDesc.Quality = 0;
		LightIndicesBufferResourceDesc.Width = ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;

		D3D12_RESOURCE_DESC PointLightsBufferResourceDesc;
		ZeroMemory(&PointLightsBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		PointLightsBufferResourceDesc.Alignment = 0;
		PointLightsBufferResourceDesc.DepthOrArraySize = 1;
		PointLightsBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		PointLightsBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		PointLightsBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		PointLightsBufferResourceDesc.Height = 1;
		PointLightsBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		PointLightsBufferResourceDesc.MipLevels = 1;
		PointLightsBufferResourceDesc.SampleDesc.Count = 1;
		PointLightsBufferResourceDesc.SampleDesc.Quality = 0;
		PointLightsBufferResourceDesc.Width = 10000 * sizeof(PointLight);

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T HDRSceneColorTextureOffset = GetOffsetForResource(HDRSceneColorTextureResourceDesc, HeapDesc);
		SIZE_T GPULightingConstantBufferOffset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T GPUClusteredShadingConstantBufferOffset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T GPULightClustersBufferOffset = GetOffsetForResource(LightClustersBufferResourceDesc, HeapDesc);
		SIZE_T GPULightIndicesBufferOffset = GetOffsetForResource(LightIndicesBufferResourceDesc, HeapDesc);
		SIZE_T GPUPointLightsBufferOffset = GetOffsetForResource(PointLightsBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory6)));
		SAFE_DX(GPUMemory6->SetName((const wchar_t*)u"Deferred Lighting Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		HDRSceneColorTexture = CreateTexture(GPUMemory6, HDRSceneColorTextureOffset, &HDRSceneColorTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"HDR Scene Color Texture");

		GPULightingConstantBuffer = CreateBuffer(GPUMemory6, GPULightingConstantBufferOffset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Lighting Constant Buffer");
		GPUClusteredShadingConstantBuffer = CreateBuffer(GPUMemory6, GPUClusteredShadingConstantBufferOffset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Clustered Shading Constant Buffer");

		GPULightClustersBuffer = CreateBuffer(GPUMemory6, GPULightClustersBufferOffset, &LightClustersBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, u"GPU Light Clusters Buffer");
		GPULightIndicesBuffer = CreateBuffer(GPUMemory6, GPULightIndicesBufferOffset, &LightIndicesBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, u"GPU Light Clusters Buffer");
		GPUPointLightsBuffer = CreateBuffer(GPUMemory6, GPUPointLightsBufferOffset, &PointLightsBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, u"GPU Point Lights Buffer");

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CPULightingConstantBuffer0Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPULightingConstantBuffer1Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUClusteredShadingConstantBuffer0Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUClusteredShadingConstantBuffer1Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPULightClustersBufferOffset0 = GetOffsetForResource(LightClustersBufferResourceDesc, HeapDesc);
		SIZE_T CPULightClustersBufferOffset1 = GetOffsetForResource(LightClustersBufferResourceDesc, HeapDesc);
		SIZE_T CPULightIndicesBufferOffset0 = GetOffsetForResource(LightIndicesBufferResourceDesc, HeapDesc);
		SIZE_T CPULightIndicesBufferOffset1 = GetOffsetForResource(LightIndicesBufferResourceDesc, HeapDesc);
		SIZE_T CPUPointLightsBufferOffset0 = GetOffsetForResource(PointLightsBufferResourceDesc, HeapDesc);
		SIZE_T CPUPointLightsBufferOffset1 = GetOffsetForResource(PointLightsBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory6)));
		SAFE_DX(CPUMemory6->SetName((const wchar_t*)u"Deferred Lighting Pass Data GPU Heap"));

		CPULightingConstantBuffers[0] = CreateBuffer(CPUMemory6, CPULightingConstantBuffer0Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Lighting Constant Buffer 0");
		CPULightingConstantBuffers[1] = CreateBuffer(CPUMemory6, CPULightingConstantBuffer1Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Lighting Constant Buffer 1");
		CPUClusteredShadingConstantBuffers[0] = CreateBuffer(CPUMemory6, CPUClusteredShadingConstantBuffer0Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Clustered Shading Constant Buffer 0");
		CPUClusteredShadingConstantBuffers[1] = CreateBuffer(CPUMemory6, CPUClusteredShadingConstantBuffer1Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Clustered Shading Constant Buffer 1");

		CPULightClustersBuffers[0] = CreateBuffer(CPUMemory6, CPULightClustersBufferOffset0, &LightClustersBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Light Clusters Buffer 0");
		CPULightClustersBuffers[1] = CreateBuffer(CPUMemory6, CPULightClustersBufferOffset1, &LightClustersBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Light Clusters Buffer 1");
		CPULightIndicesBuffers[0] = CreateBuffer(CPUMemory6, CPULightIndicesBufferOffset0, &LightIndicesBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Light Clusters Buffer 0");
		CPULightIndicesBuffers[1] = CreateBuffer(CPUMemory6, CPULightIndicesBufferOffset1, &LightIndicesBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Light Clusters Buffer 1");
		CPUPointLightsBuffers[0] = CreateBuffer(CPUMemory6, CPUPointLightsBufferOffset0, &PointLightsBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Point Lights Buffer 0");
		CPUPointLightsBuffers[1] = CreateBuffer(CPUMemory6, CPUPointLightsBufferOffset1, &PointLightsBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Point Lights Buffer 1");

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

		HDRSceneColorTextureRTV = RTDescriptorHeap->AllocateDescriptor();

		Device->CreateRenderTargetView(HDRSceneColorTexture->DXTexture, &RTVDesc, HDRSceneColorTextureRTV);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

		HDRSceneColorTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(HDRSceneColorTexture->DXTexture, &SRVDesc, HDRSceneColorTextureSRV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPULightingConstantBuffer->DXBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		LightingConstantBufferCBV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateConstantBufferView(&CBVDesc, LightingConstantBufferCBV);

		CBVDesc.BufferLocation = GPUClusteredShadingConstantBuffer->DXBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		ClusteredShadingConstantBufferCBV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateConstantBufferView(&CBVDesc, ClusteredShadingConstantBufferCBV);

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
		SRVDesc.Buffer.NumElements = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
		SRVDesc.Buffer.StructureByteStride = 0;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

		LightClustersBufferSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(GPULightClustersBuffer->DXBuffer, &SRVDesc, LightClustersBufferSRV);

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
		SRVDesc.Buffer.NumElements = ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
		SRVDesc.Buffer.StructureByteStride = 0;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

		LightIndicesBufferSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(GPULightIndicesBuffer->DXBuffer, &SRVDesc, LightIndicesBufferSRV);

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
		SRVDesc.Buffer.NumElements = 10000;
		SRVDesc.Buffer.StructureByteStride = sizeof(PointLight);
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

		PointLightsBufferSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(GPUPointLightsBuffer->DXBuffer, &SRVDesc, PointLightsBufferSRV);

		void *DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.DeferredLighting");
		SIZE_T DeferredLightingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.DeferredLighting");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = DeferredLightingPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = DeferredLightingPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 8;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DeferredLightingPipelineState)));
		SAFE_DX(DeferredLightingPipelineState->SetName((const wchar_t*)u"Deferred Lighting Pipeline State"));
	}

	// ===============================================================================================================

	{
		UINT SkyMeshVertexCount = 1 + 25 * 100 + 1;
		UINT SkyMeshIndexCount = 300 + 24 * 600 + 300;

		Vertex SkyMeshVertices[1 + 25 * 100 + 1];
		WORD SkyMeshIndices[300 + 24 * 600 + 300];

		SkyMeshVertices[0].Position = XMFLOAT3(0.0f, 1.0f, 0.0f);
		SkyMeshVertices[0].TexCoord = XMFLOAT2(0.5f, 0.5f);

		SkyMeshVertices[1 + 25 * 100].Position = XMFLOAT3(0.0f, -1.0f, 0.0f);
		SkyMeshVertices[1 + 25 * 100].TexCoord = XMFLOAT2(0.5f, 0.5f);

		for (int i = 0; i < 100; i++)
		{
			for (int j = 0; j < 25; j++)
			{
				float Theta = ((j + 1) / 25.0f) * 0.5f * 3.1416f;
				float Phi = (i / 100.0f) * 2.0f * 3.1416f;

				float X = sinf(Theta) * cosf(Phi);
				float Y = sinf(Theta) * sinf(Phi);
				float Z = cosf(Theta);

				SkyMeshVertices[1 + j * 100 + i].Position = XMFLOAT3(X, Z, Y);
				SkyMeshVertices[1 + j * 100 + i].TexCoord = XMFLOAT2(X * 0.5f + 0.5f, Y * 0.5f + 0.5f);
			}
		}

		for (int i = 0; i < 100; i++)
		{
			SkyMeshIndices[3 * i] = 0;
			SkyMeshIndices[3 * i + 1] = i + 1;
			SkyMeshIndices[3 * i + 2] = i != 99 ? i + 2 : 1;
		}

		for (int j = 0; j < 24; j++)
		{
			for (int i = 0; i < 100; i++)
			{
				SkyMeshIndices[300 + j * 600 + 6 * i] = 1 + i + j * 100;
				SkyMeshIndices[300 + j * 600 + 6 * i + 1] = 1 + i + (j + 1) * 100;
				SkyMeshIndices[300 + j * 600 + 6 * i + 2] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
				SkyMeshIndices[300 + j * 600 + 6 * i + 3] = 1 + i + j * 100;
				SkyMeshIndices[300 + j * 600 + 6 * i + 4] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
				SkyMeshIndices[300 + j * 600 + 6 * i + 5] = i != 99 ? 1 + i + 1 + j * 100 : 1 + j * 100;
			}
		}

		for (int i = 0; i < 100; i++)
		{
			SkyMeshIndices[300 + 24 * 600 + 3 * i] = 1 + 25 * 100;
			SkyMeshIndices[300 + 24 * 600 + 3 * i + 1] = i != 99 ? 1 + 24 * 100 + i + 1 : 1 + 24 * 100;
			SkyMeshIndices[300 + 24 * 600 + 3 * i + 2] = 1 + 24 * 100 + i;
		}

		UINT SunMeshVertexCount = 4;
		UINT SunMeshIndexCount = 6;

		Vertex SunMeshVertices[4] = {

			{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }

		};

		WORD SunMeshIndices[6] = { 0, 1, 2, 2, 1, 3 };

		D3D12_RESOURCE_DESC SkyVertexBufferResourceDesc;
		ZeroMemory(&SkyVertexBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SkyVertexBufferResourceDesc.Alignment = 0;
		SkyVertexBufferResourceDesc.DepthOrArraySize = 1;
		SkyVertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		SkyVertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SkyVertexBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SkyVertexBufferResourceDesc.Height = 1;
		SkyVertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		SkyVertexBufferResourceDesc.MipLevels = 1;
		SkyVertexBufferResourceDesc.SampleDesc.Count = 1;
		SkyVertexBufferResourceDesc.SampleDesc.Quality = 0;
		SkyVertexBufferResourceDesc.Width = sizeof(Vertex) * SkyMeshVertexCount;

		D3D12_RESOURCE_DESC SkyIndexBufferResourceDesc;
		ZeroMemory(&SkyIndexBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SkyIndexBufferResourceDesc.Alignment = 0;
		SkyIndexBufferResourceDesc.DepthOrArraySize = 1;
		SkyIndexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		SkyIndexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SkyIndexBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SkyIndexBufferResourceDesc.Height = 1;
		SkyIndexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		SkyIndexBufferResourceDesc.MipLevels = 1;
		SkyIndexBufferResourceDesc.SampleDesc.Count = 1;
		SkyIndexBufferResourceDesc.SampleDesc.Quality = 0;
		SkyIndexBufferResourceDesc.Width = sizeof(WORD) * SkyMeshIndexCount;

		D3D12_RESOURCE_DESC SkyConstantBufferResourceDesc;
		ZeroMemory(&SkyConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SkyConstantBufferResourceDesc.Alignment = 0;
		SkyConstantBufferResourceDesc.DepthOrArraySize = 1;
		SkyConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		SkyConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SkyConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SkyConstantBufferResourceDesc.Height = 1;
		SkyConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		SkyConstantBufferResourceDesc.MipLevels = 1;
		SkyConstantBufferResourceDesc.SampleDesc.Count = 1;
		SkyConstantBufferResourceDesc.SampleDesc.Quality = 0;
		SkyConstantBufferResourceDesc.Width = 256;

		D3D12_RESOURCE_DESC SkyTextureResourceDesc;
		ZeroMemory(&SkyTextureResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SkyTextureResourceDesc.Alignment = 0;
		SkyTextureResourceDesc.DepthOrArraySize = 1;
		SkyTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		SkyTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SkyTextureResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SkyTextureResourceDesc.Height = 2048;
		SkyTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		SkyTextureResourceDesc.MipLevels = 1;
		SkyTextureResourceDesc.SampleDesc.Count = 1;
		SkyTextureResourceDesc.SampleDesc.Quality = 0;
		SkyTextureResourceDesc.Width = 2048;

		D3D12_RESOURCE_DESC SunVertexBufferResourceDesc;
		ZeroMemory(&SunVertexBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SunVertexBufferResourceDesc.Alignment = 0;
		SunVertexBufferResourceDesc.DepthOrArraySize = 1;
		SunVertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		SunVertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SunVertexBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SunVertexBufferResourceDesc.Height = 1;
		SunVertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		SunVertexBufferResourceDesc.MipLevels = 1;
		SunVertexBufferResourceDesc.SampleDesc.Count = 1;
		SunVertexBufferResourceDesc.SampleDesc.Quality = 0;
		SunVertexBufferResourceDesc.Width = sizeof(Vertex) * SunMeshVertexCount;

		D3D12_RESOURCE_DESC SunIndexBufferResourceDesc;
		ZeroMemory(&SunIndexBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SunIndexBufferResourceDesc.Alignment = 0;
		SunIndexBufferResourceDesc.DepthOrArraySize = 1;
		SunIndexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		SunIndexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SunIndexBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SunIndexBufferResourceDesc.Height = 1;
		SunIndexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		SunIndexBufferResourceDesc.MipLevels = 1;
		SunIndexBufferResourceDesc.SampleDesc.Count = 1;
		SunIndexBufferResourceDesc.SampleDesc.Quality = 0;
		SunIndexBufferResourceDesc.Width = sizeof(WORD) * SunMeshIndexCount;

		D3D12_RESOURCE_DESC SunConstantBufferResourceDesc;
		ZeroMemory(&SunConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SunConstantBufferResourceDesc.Alignment = 0;
		SunConstantBufferResourceDesc.DepthOrArraySize = 1;
		SunConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		SunConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SunConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SunConstantBufferResourceDesc.Height = 1;
		SunConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		SunConstantBufferResourceDesc.MipLevels = 1;
		SunConstantBufferResourceDesc.SampleDesc.Count = 1;
		SunConstantBufferResourceDesc.SampleDesc.Quality = 0;
		SunConstantBufferResourceDesc.Width = 256;

		D3D12_RESOURCE_DESC SunTextureResourceDesc;
		ZeroMemory(&SunTextureResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		SunTextureResourceDesc.Alignment = 0;
		SunTextureResourceDesc.DepthOrArraySize = 1;
		SunTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		SunTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		SunTextureResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SunTextureResourceDesc.Height = 512;
		SunTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		SunTextureResourceDesc.MipLevels = 1;
		SunTextureResourceDesc.SampleDesc.Count = 1;
		SunTextureResourceDesc.SampleDesc.Quality = 0;
		SunTextureResourceDesc.Width = 512;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T SkyVertexBufferOffset = GetOffsetForResource(SkyVertexBufferResourceDesc, HeapDesc);
		SIZE_T SkyIndexBufferOffset = GetOffsetForResource(SkyIndexBufferResourceDesc, HeapDesc);
		SIZE_T SunVertexBufferOffset = GetOffsetForResource(SunVertexBufferResourceDesc, HeapDesc);
		SIZE_T SunIndexBufferOffset = GetOffsetForResource(SunIndexBufferResourceDesc, HeapDesc);
		SIZE_T SkyTextureOffset = GetOffsetForResource(SkyTextureResourceDesc, HeapDesc);
		SIZE_T SunTextureOffset = GetOffsetForResource(SunTextureResourceDesc, HeapDesc);
		SIZE_T GPUSkyConstantBufferOffset = GetOffsetForResource(SkyConstantBufferResourceDesc, HeapDesc);
		SIZE_T GPUSunConstantBufferOffset = GetOffsetForResource(SunConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory7)));
		SAFE_DX(GPUMemory7->SetName((const wchar_t*)u"Sky and Fog Pass Data GPU Heap"));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SkyVertexBufferOffset, &SkyVertexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(SkyVertexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SkyIndexBufferOffset, &SkyIndexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(SkyIndexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SkyTextureOffset, &SkyTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(SkyTexture)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SunVertexBufferOffset, &SunVertexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(SunVertexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SunIndexBufferOffset, &SunIndexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(SunIndexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SunTextureOffset, &SunTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(SunTexture)));
		SAFE_DX(SkyVertexBuffer->SetName((const wchar_t*)u"Sky Vertex Buffer"));
		SAFE_DX(SkyIndexBuffer->SetName((const wchar_t*)u"Sky Index Buffer"));
		SAFE_DX(SkyTexture->SetName((const wchar_t*)u"Sky Texture"));
		SAFE_DX(SunVertexBuffer->SetName((const wchar_t*)u"Sun Vertex Buffer"));
		SAFE_DX(SunIndexBuffer->SetName((const wchar_t*)u"Sun Index Buffer"));
		SAFE_DX(SunTexture->SetName((const wchar_t*)u"Sun Texture"));

		GPUSkyConstantBuffer = CreateBuffer(GPUMemory7, GPUSkyConstantBufferOffset, &SkyConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Sky Constant Buffer");
		GPUSunConstantBuffer = CreateBuffer(GPUMemory7, GPUSunConstantBufferOffset, &SunConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"GPU Sun Constant Buffer");

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CPUSkyConstantBufferOffset0 = GetOffsetForResource(SkyConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUSkyConstantBufferOffset1 = GetOffsetForResource(SkyConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUSunConstantBufferOffset0 = GetOffsetForResource(SunConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUSunConstantBufferOffset1 = GetOffsetForResource(SunConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory7)));
		SAFE_DX(CPUMemory7->SetName((const wchar_t*)u"Sky and Fog Pass Data GPU Heap"));

		CPUSkyConstantBuffers[0] = CreateBuffer(CPUMemory7, CPUSkyConstantBufferOffset0, &SkyConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Sky Constant Buffers 0");
		CPUSkyConstantBuffers[1] = CreateBuffer(CPUMemory7, CPUSkyConstantBufferOffset1, &SkyConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Sky Constant Buffers 1");
		CPUSunConstantBuffers[0] = CreateBuffer(CPUMemory7, CPUSunConstantBufferOffset0, &SunConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Sun Constant Buffers 0");
		CPUSunConstantBuffers[1] = CreateBuffer(CPUMemory7, CPUSunConstantBufferOffset1, &SunConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Sun Constant Buffers 1");

		void *MappedData;

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		WrittenRange.Begin = 0;
		WrittenRange.End = sizeof(Vertex) * SkyMeshVertexCount;

		SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
		memcpy((BYTE*)MappedData, SkyMeshVertices, sizeof(Vertex) * SkyMeshVertexCount);
		memcpy((BYTE*)MappedData + sizeof(Vertex) * SkyMeshVertexCount, SkyMeshIndices, sizeof(WORD) * SkyMeshIndexCount);
		UploadBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CopyCommandAllocator->Reset());
		SAFE_DX(CopyCommandList->Reset(CopyCommandAllocator, nullptr));

		CopyCommandList->CopyBufferRegion(SkyVertexBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * SkyMeshVertexCount);
		CopyCommandList->CopyBufferRegion(SkyIndexBuffer, 0, UploadBuffer, sizeof(Vertex) * SkyMeshVertexCount, sizeof(WORD) * SkyMeshIndexCount);

		/*D3D12_RESOURCE_BARRIER ResourceBarriers[2];
		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SkyVertexBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SkyIndexBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);*/

		SAFE_DX(CopyCommandList->Close());

		CopyCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CopyCommandList);

		SAFE_DX(CopyCommandQueue->Signal(CopySyncFence, 1));

		if (CopySyncFence->GetCompletedValue() != 1)
		{
			SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
			DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
		}

		SAFE_DX(CopySyncFence->Signal(0));

		SkyVertexBufferAddress = SkyVertexBuffer->GetGPUVirtualAddress();
		SkyIndexBufferAddress = SkyIndexBuffer->GetGPUVirtualAddress();

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUSkyConstantBuffer->DXBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		SkyConstantBufferCBV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateConstantBufferView(&CBVDesc, SkyConstantBufferCBV);

		void *SkyVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.SkyVertexShader");
		SIZE_T SkyVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.SkyVertexShader");

		void *SkyPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.SkyPixelShader");
		SIZE_T SkyPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.SkyPixelShader");

		D3D12_INPUT_ELEMENT_DESC InputElementDescs[5];
		ZeroMemory(InputElementDescs, 5 * sizeof(D3D12_INPUT_ELEMENT_DESC));
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
		InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDescs[2].InputSlot = 0;
		InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDescs[2].InstanceDataStepRate = 0;
		InputElementDescs[2].SemanticIndex = 0;
		InputElementDescs[2].SemanticName = "NORMAL";
		InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDescs[3].InputSlot = 0;
		InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDescs[3].InstanceDataStepRate = 0;
		InputElementDescs[3].SemanticIndex = 0;
		InputElementDescs[3].SemanticName = "TANGENT";
		InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDescs[4].InputSlot = 0;
		InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDescs[4].InstanceDataStepRate = 0;
		InputElementDescs[4].SemanticIndex = 0;
		InputElementDescs[4].SemanticName = "BINORMAL";

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
		GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
		GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
		GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = SkyPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = SkyPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 8;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = SkyVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = SkyVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SkyPipelineState)));
		SAFE_DX(SkyPipelineState->SetName((const wchar_t*)u"Sky Pipeline State"));

		Pointer<Texel[]> SkyTextureTexels = Pointer<Texel[]>::Create(2048 * 2048);

		for (int y = 0; y < 2048; y++)
		{
			for (int x = 0; x < 2048; x++)
			{
				float X = (x / 2048.0f) * 2.0f - 1.0f;
				float Y = (y / 2048.0f) * 2.0f - 1.0f;

				float D = sqrtf(X * X + Y * Y);

				SkyTextureTexels[y * 2048 + x].R = 0;
				SkyTextureTexels[y * 2048 + x].G = 128 * D < 255 ? BYTE(128 * D) : 255;
				SkyTextureTexels[y * 2048 + x].B = 255;
				SkyTextureTexels[y * 2048 + x].A = 255;
			}
		}

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

		UINT NumRows;
		UINT64 RowSizeInBytes, TotalBytes;

		Device->GetCopyableFootprints(&SkyTextureResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

		ReadRange.Begin = 0;
		ReadRange.End = 0;

		WrittenRange.Begin = 0;
		WrittenRange.End = TotalBytes;

		SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));

		BYTE *TexelData = (BYTE*)SkyTextureTexels;

		for (UINT j = 0; j < NumRows; j++)
		{
			memcpy((BYTE*)MappedData + PlacedSubResourceFootPrint.Offset + j * PlacedSubResourceFootPrint.Footprint.RowPitch, (BYTE*)TexelData + j * RowSizeInBytes, RowSizeInBytes);
		}

		UploadBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CopyCommandAllocator->Reset());
		SAFE_DX(CopyCommandList->Reset(CopyCommandAllocator, nullptr));

		D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

		SourceTextureCopyLocation.pResource = UploadBuffer;
		SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		DestTextureCopyLocation.pResource = SkyTexture;
		DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
		DestTextureCopyLocation.SubresourceIndex = 0;

		CopyCommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

		/*D3D12_RESOURCE_BARRIER ResourceBarrier;
		ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarrier.Transition.pResource = SkyTexture;
		ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CopyCommandList->ResourceBarrier(1, &ResourceBarrier);*/

		SAFE_DX(CopyCommandList->Close());

		CopyCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CopyCommandList);

		SAFE_DX(CopyCommandQueue->Signal(CopySyncFence, 1));

		if (CopySyncFence->GetCompletedValue() != 1)
		{
			SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
			DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
		}

		SAFE_DX(CopySyncFence->Signal(0));

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		SkyTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(SkyTexture, &SRVDesc, SkyTextureSRV);

		ReadRange.Begin = 0;
		ReadRange.End = 0;

		WrittenRange.Begin = 0;
		WrittenRange.End = sizeof(Vertex) * SunMeshVertexCount + sizeof(WORD) * SunMeshIndexCount;

		SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
		memcpy((BYTE*)MappedData, SunMeshVertices, sizeof(Vertex) * SunMeshVertexCount);
		memcpy((BYTE*)MappedData + sizeof(Vertex) * SunMeshVertexCount, SunMeshIndices, sizeof(WORD) * SunMeshIndexCount);
		UploadBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CopyCommandAllocator->Reset());
		SAFE_DX(CopyCommandList->Reset(CopyCommandAllocator, nullptr));

		CopyCommandList->CopyBufferRegion(SunVertexBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * SunMeshVertexCount);
		CopyCommandList->CopyBufferRegion(SunIndexBuffer, 0, UploadBuffer, sizeof(Vertex) * SunMeshVertexCount, sizeof(WORD) * SunMeshIndexCount);

		/*ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SunVertexBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SunIndexBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);*/

		SAFE_DX(CopyCommandList->Close());

		CopyCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CopyCommandList);

		SAFE_DX(CopyCommandQueue->Signal(CopySyncFence, 1));

		if (CopySyncFence->GetCompletedValue() != 1)
		{
			SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
			DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
		}

		SAFE_DX(CopySyncFence->Signal(0));

		SunVertexBufferAddress = SunVertexBuffer->GetGPUVirtualAddress();
		SunIndexBufferAddress = SunIndexBuffer->GetGPUVirtualAddress();

		CBVDesc.BufferLocation = GPUSunConstantBuffer->DXBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		SunConstantBufferCBV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateConstantBufferView(&CBVDesc, SunConstantBufferCBV);

		void *SunVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.SunVertexShader");
		SIZE_T SunVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.SunVertexShader");

		void *SunPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.SunPixelShader");
		SIZE_T SunPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.SunPixelShader");

		ZeroMemory(InputElementDescs, 5 * sizeof(D3D12_INPUT_ELEMENT_DESC));
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
		InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDescs[2].InputSlot = 0;
		InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDescs[2].InstanceDataStepRate = 0;
		InputElementDescs[2].SemanticIndex = 0;
		InputElementDescs[2].SemanticName = "NORMAL";
		InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDescs[3].InputSlot = 0;
		InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDescs[3].InstanceDataStepRate = 0;
		InputElementDescs[3].SemanticIndex = 0;
		InputElementDescs[3].SemanticName = "TANGENT";
		InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDescs[4].InputSlot = 0;
		InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDescs[4].InstanceDataStepRate = 0;
		InputElementDescs[4].SemanticIndex = 0;
		InputElementDescs[4].SemanticName = "BINORMAL";

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
		GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
		GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
		GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
		GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = SunPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = SunPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 8;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = SunVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = SunVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SunPipelineState)));
		SAFE_DX(SunPipelineState->SetName((const wchar_t*)u"Sun Pipeline State"));

		Pointer<Texel[]> SunTextureTexels = Pointer<Texel[]>::Create(512 * 512);

		for (int y = 0; y < 512; y++)
		{
			for (int x = 0; x < 512; x++)
			{
				float X = (x / 512.0f) * 2.0f - 1.0f;
				float Y = (y / 512.0f) * 2.0f - 1.0f;

				float D = sqrtf(X * X + Y * Y);

				SunTextureTexels[y * 512 + x].R = 255;
				SunTextureTexels[y * 512 + x].G = 255;
				SunTextureTexels[y * 512 + x].B = 127 + 128 * D < 255 ? BYTE(127 + 128 * D) : 255;
				SunTextureTexels[y * 512 + x].A = 255 * D < 255 ? BYTE(255 * D) : 255;
			}
		}

		Device->GetCopyableFootprints(&SunTextureResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

		ReadRange.Begin = 0;
		ReadRange.End = 0;

		WrittenRange.Begin = 0;
		WrittenRange.End = TotalBytes;

		SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));

		TexelData = (BYTE*)SunTextureTexels;

		for (UINT j = 0; j < NumRows; j++)
		{
			memcpy((BYTE*)MappedData + PlacedSubResourceFootPrint.Offset + j * PlacedSubResourceFootPrint.Footprint.RowPitch, (BYTE*)TexelData + j * RowSizeInBytes, RowSizeInBytes);
		}

		UploadBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CopyCommandAllocator->Reset());
		SAFE_DX(CopyCommandList->Reset(CopyCommandAllocator, nullptr));

		SourceTextureCopyLocation.pResource = UploadBuffer;
		SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		DestTextureCopyLocation.pResource = SunTexture;
		DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
		DestTextureCopyLocation.SubresourceIndex = 0;

		CopyCommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

		/*ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarrier.Transition.pResource = SunTexture;
		ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CopyCommandList->ResourceBarrier(1, &ResourceBarrier);*/

		SAFE_DX(CopyCommandList->Close());

		CopyCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CopyCommandList);

		SAFE_DX(CopyCommandQueue->Signal(CopySyncFence, 1));

		if (CopySyncFence->GetCompletedValue() != 1)
		{
			SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
			DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
		}

		SAFE_DX(CopySyncFence->Signal(0));

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		SunTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(SunTexture, &SRVDesc, SunTextureSRV);

		void *FogPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.Fog");
		SIZE_T FogPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.Fog");

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = FogPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = FogPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 8;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(FogPipelineState)));
		SAFE_DX(FogPipelineState->SetName((const wchar_t*)u"Fog Pipeline State"));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ResourceDesc;
		ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ResourceDesc.Alignment = 0;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		ResourceDesc.Height = ResolutionHeight;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Width = ResolutionWidth;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T ResolvedHDRSceneColorTextureOffset = GetOffsetForResource(ResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory8)));
		SAFE_DX(GPUMemory1->SetName((const wchar_t*)u"HDR Scene Color Texture Resolve Pass Data GPU Heap"));

		ResolvedHDRSceneColorTexture = CreateTexture(GPUMemory8, ResolvedHDRSceneColorTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, u"Resolved HDR Scene Color Texture");

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		ResolvedHDRSceneColorTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(ResolvedHDRSceneColorTexture->DXTexture, &SRVDesc, ResolvedHDRSceneColorTextureSRV);
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC SceneLuminanceTexturesResourceDescs[12];

		int Widths[12] = { 1280, 640, 320, 160, 80, 40, 20, 10, 5, 3, 2, 1 };
		int Heights[12] = { 720, 360, 180, 90, 45, 23, 12, 6, 3, 2, 1, 1 };

		for (int i = 0; i < 12; i++)
		{
			ZeroMemory(&SceneLuminanceTexturesResourceDescs[i], sizeof(D3D12_RESOURCE_DESC));
			SceneLuminanceTexturesResourceDescs[i].Alignment = 0;
			SceneLuminanceTexturesResourceDescs[i].DepthOrArraySize = 1;
			SceneLuminanceTexturesResourceDescs[i].Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			SceneLuminanceTexturesResourceDescs[i].Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			SceneLuminanceTexturesResourceDescs[i].Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
			SceneLuminanceTexturesResourceDescs[i].Height = Heights[i];
			SceneLuminanceTexturesResourceDescs[i].Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
			SceneLuminanceTexturesResourceDescs[i].MipLevels = 1;
			SceneLuminanceTexturesResourceDescs[i].SampleDesc.Count = 1;
			SceneLuminanceTexturesResourceDescs[i].SampleDesc.Quality = 0;
			SceneLuminanceTexturesResourceDescs[i].Width = Widths[i];
		}

		D3D12_RESOURCE_DESC AverageLuminanceTextureResourceDesc;
		ZeroMemory(&AverageLuminanceTextureResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		AverageLuminanceTextureResourceDesc.Alignment = 0;
		AverageLuminanceTextureResourceDesc.DepthOrArraySize = 1;
		AverageLuminanceTextureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		AverageLuminanceTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		AverageLuminanceTextureResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		AverageLuminanceTextureResourceDesc.Height = 1;
		AverageLuminanceTextureResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		AverageLuminanceTextureResourceDesc.MipLevels = 1;
		AverageLuminanceTextureResourceDesc.SampleDesc.Count = 1;
		AverageLuminanceTextureResourceDesc.SampleDesc.Quality = 0;
		AverageLuminanceTextureResourceDesc.Width = 1;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T SceneLuminanceTexture0Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[0], HeapDesc);
		SIZE_T SceneLuminanceTexture1Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[1], HeapDesc);
		SIZE_T SceneLuminanceTexture2Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[2], HeapDesc);
		SIZE_T SceneLuminanceTexture3Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[3], HeapDesc);
		SIZE_T SceneLuminanceTexture4Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[4], HeapDesc);
		SIZE_T SceneLuminanceTexture5Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[5], HeapDesc);
		SIZE_T SceneLuminanceTexture6Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[6], HeapDesc);
		SIZE_T SceneLuminanceTexture7Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[7], HeapDesc);
		SIZE_T SceneLuminanceTexture8Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[8], HeapDesc);
		SIZE_T SceneLuminanceTexture9Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[9], HeapDesc);
		SIZE_T SceneLuminanceTexture10Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[10], HeapDesc);
		SIZE_T SceneLuminanceTexture11Offset = GetOffsetForResource(SceneLuminanceTexturesResourceDescs[11], HeapDesc);
		SIZE_T AverageLuminanceTextureOffset = GetOffsetForResource(AverageLuminanceTextureResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory9)));
		SAFE_DX(GPUMemory9->SetName((const wchar_t*)u"Scene Luminance Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

		SceneLuminanceTextures[0] = CreateTexture(GPUMemory9, SceneLuminanceTexture0Offset, &SceneLuminanceTexturesResourceDescs[0], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 0");
		SceneLuminanceTextures[1] = CreateTexture(GPUMemory9, SceneLuminanceTexture1Offset, &SceneLuminanceTexturesResourceDescs[1], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 1");
		SceneLuminanceTextures[2] = CreateTexture(GPUMemory9, SceneLuminanceTexture2Offset, &SceneLuminanceTexturesResourceDescs[2], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 2");
		SceneLuminanceTextures[3] = CreateTexture(GPUMemory9, SceneLuminanceTexture3Offset, &SceneLuminanceTexturesResourceDescs[3], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 3");
		SceneLuminanceTextures[4] = CreateTexture(GPUMemory9, SceneLuminanceTexture4Offset, &SceneLuminanceTexturesResourceDescs[4], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 4");
		SceneLuminanceTextures[5] = CreateTexture(GPUMemory9, SceneLuminanceTexture5Offset, &SceneLuminanceTexturesResourceDescs[5], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 5");
		SceneLuminanceTextures[6] = CreateTexture(GPUMemory9, SceneLuminanceTexture6Offset, &SceneLuminanceTexturesResourceDescs[6], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 6");
		SceneLuminanceTextures[7] = CreateTexture(GPUMemory9, SceneLuminanceTexture7Offset, &SceneLuminanceTexturesResourceDescs[7], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 7");
		SceneLuminanceTextures[8] = CreateTexture(GPUMemory9, SceneLuminanceTexture8Offset, &SceneLuminanceTexturesResourceDescs[8], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 8");
		SceneLuminanceTextures[9] = CreateTexture(GPUMemory9, SceneLuminanceTexture9Offset, &SceneLuminanceTexturesResourceDescs[9], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 9");
		SceneLuminanceTextures[10] = CreateTexture(GPUMemory9, SceneLuminanceTexture10Offset, &SceneLuminanceTexturesResourceDescs[10], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 10");
		SceneLuminanceTextures[11] = CreateTexture(GPUMemory9, SceneLuminanceTexture11Offset, &SceneLuminanceTexturesResourceDescs[11], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Scene Luminance Textures 11");
		
		AverageLuminanceTexture = CreateTexture(GPUMemory9, AverageLuminanceTextureOffset, &AverageLuminanceTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Average Luminance Texture");

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 12; i++)
		{
			SceneLuminanceTexturesRTVs[i] = RTDescriptorHeap->AllocateDescriptor();

			Device->CreateRenderTargetView(SceneLuminanceTextures[i]->DXTexture, &RTVDesc, SceneLuminanceTexturesRTVs[i]);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 12; i++)
		{
			SceneLuminanceTexturesSRVs[i] = CBSRUADescriptorHeap->AllocateDescriptor();

			Device->CreateShaderResourceView(SceneLuminanceTextures[i]->DXTexture, &SRVDesc, SceneLuminanceTexturesSRVs[i]);
		}

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		AverageLuminanceTextureRTV = RTDescriptorHeap->AllocateDescriptor();

		Device->CreateRenderTargetView(AverageLuminanceTexture->DXTexture, &RTVDesc, AverageLuminanceTextureRTV);

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		AverageLuminanceTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(AverageLuminanceTexture->DXTexture, &SRVDesc, AverageLuminanceTextureSRV);

		void *LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.LuminanceCalc");
		SIZE_T LuminanceCalcComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.LuminanceCalc");

		void *LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.LuminanceSum");
		SIZE_T LuminanceSumComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.LuminanceSum");

		void *LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.LuminanceAvg");
		SIZE_T LuminanceAvgComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.LuminanceAvg");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = LuminanceCalcComputeShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = LuminanceCalcComputeShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(LuminanceCalcPipelineState)));
		SAFE_DX(LuminanceCalcPipelineState->SetName((const wchar_t*)u"Luminance Calc Pipeline State"));

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = LuminanceSumComputeShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = LuminanceSumComputeShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(LuminanceSumPipelineState)));
		SAFE_DX(LuminanceSumPipelineState->SetName((const wchar_t*)u"Luminance Sum Pipeline State"));

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = LuminanceAvgComputeShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = LuminanceAvgComputeShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(LuminanceAvgPipelineState)));
		SAFE_DX(LuminanceAvgPipelineState->SetName((const wchar_t*)u"Luminance Avg Pipeline State"));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ResourceDescs[3][7];

		for (int i = 0; i < 7; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				ZeroMemory(&ResourceDescs[j][i], sizeof(D3D12_RESOURCE_DESC));
				ResourceDescs[j][i].Alignment = 0;
				ResourceDescs[j][i].DepthOrArraySize = 1;
				ResourceDescs[j][i].Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				ResourceDescs[j][i].Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
				ResourceDescs[j][i].Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
				ResourceDescs[j][i].Height = ResolutionHeight >> i;
				ResourceDescs[j][i].Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
				ResourceDescs[j][i].MipLevels = 1;
				ResourceDescs[j][i].SampleDesc.Count = 1;
				ResourceDescs[j][i].SampleDesc.Quality = 0;
				ResourceDescs[j][i].Width = ResolutionWidth >> i;
			}
		}

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T BloomTexturesOffsets[3][7];

		for (int i = 0; i < 7; i++)
		{
			BloomTexturesOffsets[0][i] = GetOffsetForResource(ResourceDescs[0][i], HeapDesc);
			BloomTexturesOffsets[1][i] = GetOffsetForResource(ResourceDescs[1][i], HeapDesc);
			BloomTexturesOffsets[2][i] = GetOffsetForResource(ResourceDescs[2][i], HeapDesc);
		}

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory10)));
		SAFE_DX(GPUMemory10->SetName((const wchar_t*)u"Bloom Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

		for (int i = 0; i < 7; i++)
		{
			char16_t DebugTextureName[255];
			wsprintf((wchar_t*)DebugTextureName, (const wchar_t*)u"Bloom Texture %d %d", i, 0);
			BloomTextures[0][i] = CreateTexture(GPUMemory10, BloomTexturesOffsets[0][i], &ResourceDescs[0][i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, DebugTextureName);
			wsprintf((wchar_t*)DebugTextureName, (const wchar_t*)u"Bloom Texture %d %d", i, 1);
			BloomTextures[1][i] = CreateTexture(GPUMemory10, BloomTexturesOffsets[1][i], &ResourceDescs[1][i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, DebugTextureName);
			wsprintf((wchar_t*)DebugTextureName, (const wchar_t*)u"Bloom Texture %d %d", i, 2);
			BloomTextures[2][i] = CreateTexture(GPUMemory10, BloomTexturesOffsets[2][i], &ResourceDescs[2][i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, DebugTextureName);
		}

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 7; i++)
		{
			BloomTexturesRTVs[0][i] = RTDescriptorHeap->AllocateDescriptor();
			BloomTexturesRTVs[1][i] = RTDescriptorHeap->AllocateDescriptor();
			BloomTexturesRTVs[2][i] = RTDescriptorHeap->AllocateDescriptor();

			Device->CreateRenderTargetView(BloomTextures[0][i]->DXTexture, &RTVDesc, BloomTexturesRTVs[0][i]);
			Device->CreateRenderTargetView(BloomTextures[1][i]->DXTexture, &RTVDesc, BloomTexturesRTVs[1][i]);
			Device->CreateRenderTargetView(BloomTextures[2][i]->DXTexture, &RTVDesc, BloomTexturesRTVs[2][i]);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 7; i++)
		{
			BloomTexturesSRVs[0][i] = CBSRUADescriptorHeap->AllocateDescriptor();
			BloomTexturesSRVs[1][i] = CBSRUADescriptorHeap->AllocateDescriptor();
			BloomTexturesSRVs[2][i] = CBSRUADescriptorHeap->AllocateDescriptor();

			Device->CreateShaderResourceView(BloomTextures[0][i]->DXTexture, &SRVDesc, BloomTexturesSRVs[0][i]);
			Device->CreateShaderResourceView(BloomTextures[1][i]->DXTexture, &SRVDesc, BloomTexturesSRVs[1][i]);
			Device->CreateShaderResourceView(BloomTextures[2][i]->DXTexture, &SRVDesc, BloomTexturesSRVs[2][i]);
		}

		void *BrightPassPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.BrightPass");
		SIZE_T BrightPassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.BrightPass");

		void *ImageResamplePixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.ImageResample");
		SIZE_T ImageResamplePixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.ImageResample");

		void *HorizontalBlurPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.HorizontalBlur");
		SIZE_T HorizontalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.HorizontalBlur");

		void *VerticalBlurPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.VerticalBlur");
		SIZE_T VerticalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.VerticalBlur");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = BrightPassPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = BrightPassPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(BrightPassPipelineState)));
		SAFE_DX(BrightPassPipelineState->SetName((const wchar_t*)u"Bright Pass Pipeline State"));

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = ImageResamplePixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = ImageResamplePixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DownSamplePipelineState)));
		SAFE_DX(DownSamplePipelineState->SetName((const wchar_t*)u"Down Sample Pipeline State"));

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_ONE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_ONE;
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = ImageResamplePixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = ImageResamplePixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(UpSampleWithAddBlendPipelineState)));
		SAFE_DX(UpSampleWithAddBlendPipelineState->SetName((const wchar_t*)u"Up Sample With Add Blend Pipeline State"));

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = HorizontalBlurPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = HorizontalBlurPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(HorizontalBlurPipelineState)));
		SAFE_DX(HorizontalBlurPipelineState->SetName((const wchar_t*)u"Horizontal Blur Pipeline State"));

		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = VerticalBlurPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = VerticalBlurPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(VerticalBlurPipelineState)));
		SAFE_DX(VerticalBlurPipelineState->SetName((const wchar_t*)u"Vertical Blur Pipeline State"));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ResourceDesc;
		ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ResourceDesc.Alignment = 0;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		ResourceDesc.Height = ResolutionHeight;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.SampleDesc.Count = 8;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Width = ResolutionWidth;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T ToneMappedImageTextureOffset = GetOffsetForResource(ResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory11)));
		SAFE_DX(GPUMemory11->SetName((const wchar_t*)u"HDR Tone Mapping Pass Data GPU Heap"));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		ToneMappedImageTexture = CreateTexture(GPUMemory11, ToneMappedImageTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue, u"Tone Mapped Image Texture");

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

		ToneMappedImageTextureRTV = RTDescriptorHeap->AllocateDescriptor();

		Device->CreateRenderTargetView(ToneMappedImageTexture->DXTexture, &RTVDesc, ToneMappedImageTextureRTV);

		void *HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.HDRToneMapping");
		SIZE_T HDRToneMappingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.HDRToneMapping");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = HDRToneMappingPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = HDRToneMappingPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		GraphicsPipelineStateDesc.SampleDesc.Count = 8;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(HDRToneMappingPipelineState)));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ResourceDesc;
		ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ResourceDesc.Alignment = 0;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		ResourceDesc.Height = 144;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Width = 256;

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T OcclusionBufferTextureOffset = GetOffsetForResource(ResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory12)));
		SAFE_DX(GPUMemory12->SetName((const wchar_t*)u"Debug Occlusion Pass Data GPU Heap"));

		DebugOcclusionBufferTexture = CreateTexture(GPUMemory12, OcclusionBufferTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, u"Debug Occlusion Buffer Texture");

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

		UINT NumRows;
		UINT64 RowSizeInBytes, TotalBytes;

		Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

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

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T OcclusionBufferReadbackBuffer0Offset = GetOffsetForResource(ResourceDesc, HeapDesc);
		SIZE_T OcclusionBufferReadbackBuffer1Offset = GetOffsetForResource(ResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory12)));
		SAFE_DX(CPUMemory12->SetName((const wchar_t*)u"Debug Occlusion Pass Data CPU Heap"));

		DebugOcclusionBufferTextureUpload[0] = CreateBuffer(CPUMemory12, OcclusionBufferReadbackBuffer0Offset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"Debug Occlusion Buffer Texture Upload 0");
		DebugOcclusionBufferTextureUpload[1] = CreateBuffer(CPUMemory12, OcclusionBufferReadbackBuffer1Offset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"Debug Occlusion Buffer Texture Upload 1");

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		DebugOcclusionBufferTextureSRV = CBSRUADescriptorHeap->AllocateDescriptor();

		Device->CreateShaderResourceView(DebugOcclusionBufferTexture->DXTexture, &SRVDesc, DebugOcclusionBufferTextureSRV);

		void *DebugDrawOcclusionBufferVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.DebugDrawOcclusionBuffer_VertexShader");
		SIZE_T DebugDrawOcclusionBufferVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.DebugDrawOcclusionBuffer_VertexShader");
		void *DebugDrawOcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.DebugDrawOcclusionBuffer_PixelShader");
		SIZE_T DebugDrawOcclusionBufferPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.DebugDrawOcclusionBuffer_PixelShader");
		
		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = DebugDrawOcclusionBufferPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = DebugDrawOcclusionBufferPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = DebugDrawOcclusionBufferVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = DebugDrawOcclusionBufferVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DebugDrawOcclusionBufferPipelineState)));
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ConstantBufferResourceDesc;
		ZeroMemory(&ConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ConstantBufferResourceDesc.Alignment = 0;
		ConstantBufferResourceDesc.DepthOrArraySize = 1;
		ConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		ConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		ConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		ConstantBufferResourceDesc.Height = 1;
		ConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ConstantBufferResourceDesc.MipLevels = 1;
		ConstantBufferResourceDesc.SampleDesc.Count = 1;
		ConstantBufferResourceDesc.SampleDesc.Quality = 0;
		ConstantBufferResourceDesc.Width = 256 * 20000;

		D3D12_RESOURCE_DESC IndexBufferResourceDesc;
		ZeroMemory(&IndexBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		IndexBufferResourceDesc.Alignment = 0;
		IndexBufferResourceDesc.DepthOrArraySize = 1;
		IndexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		IndexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		IndexBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		IndexBufferResourceDesc.Height = 1;
		IndexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		IndexBufferResourceDesc.MipLevels = 1;
		IndexBufferResourceDesc.SampleDesc.Count = 1;
		IndexBufferResourceDesc.SampleDesc.Quality = 0;
		IndexBufferResourceDesc.Width = 24 * sizeof(uint16_t);

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T GPUConstantBufferOffset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T IndexBufferOffset = GetOffsetForResource(IndexBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory13)));
		SAFE_DX(GPUMemory13->SetName((const wchar_t*)u"Debug Bounding Boxes Pass Data GPU Heap"));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory13, IndexBufferOffset, &IndexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(BoundingBoxIndexBuffer)));
		SAFE_DX(BoundingBoxIndexBuffer->SetName((const wchar_t*)u"Bounding Box Index Buffer"));

		GPUConstantBuffer3 = CreateBuffer(GPUMemory13, GPUConstantBufferOffset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, u"Bounding Box Index Buffer");

		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CPUConstantBuffer0Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);
		SIZE_T CPUConstantBuffer1Offset = GetOffsetForResource(ConstantBufferResourceDesc, HeapDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory13)));
		SAFE_DX(CPUMemory13->SetName((const wchar_t*)u"Debug Bounding Boxes Pass Data CPU Heap"));

		CPUConstantBuffers3[0] = CreateBuffer(CPUMemory13, CPUConstantBuffer0Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Constant Buffers 3 0");
		CPUConstantBuffers3[1] = CreateBuffer(CPUMemory13, CPUConstantBuffer1Offset, &ConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, u"CPU Constant Buffers 3 1");

		for (int i = 0; i < 20000; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = GPUConstantBuffer3->DXBuffer->GetGPUVirtualAddress() + i * 256;
			CBVDesc.SizeInBytes = 256;

			ConstantBufferCBVs3[i] = ConstantBufferDescriptorHeap->AllocateDescriptor();

			Device->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs3[i]);
		}

		uint16_t BoundingBoxIndices[24] =
		{
			0, 1,
			2, 3,
			4, 5,
			6, 7,
			0, 4,
			1, 5,
			2, 6,
			3, 7,
			0, 2,
			1, 3,
			4, 6,
			5, 7
		};

		void *MappedData;

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		WrittenRange.Begin = 0;
		WrittenRange.End = 24 * sizeof(uint16_t);

		SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
		memcpy((BYTE*)MappedData, BoundingBoxIndices, 24 * sizeof(uint16_t));
		UploadBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CopyCommandAllocator->Reset());
		SAFE_DX(CopyCommandList->Reset(CopyCommandAllocator, nullptr));

		CopyCommandList->CopyBufferRegion(BoundingBoxIndexBuffer, 0, UploadBuffer, 0, 24 * sizeof(uint16_t));

		/*D3D12_RESOURCE_BARRIER ResourceBarrier;
		ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarrier.Transition.pResource = BoundingBoxIndexBuffer;
		ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
		ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarrier.Transition.Subresource = 0;
		ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CopyCommandList->ResourceBarrier(1, &ResourceBarrier);*/

		SAFE_DX(CopyCommandList->Close());

		CopyCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CopyCommandList);

		SAFE_DX(CopyCommandQueue->Signal(CopySyncFence, 1));

		if (CopySyncFence->GetCompletedValue() != 1)
		{
			SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
			DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
		}

		SAFE_DX(CopySyncFence->Signal(0));

		BoundingBoxIndexBufferAddress = BoundingBoxIndexBuffer->GetGPUVirtualAddress();

		void *DebugDrawBoundingBoxVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.DebugDrawBoundingBox_VertexShader");
		SIZE_T DebugDrawBoundingBoxVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.DebugDrawBoundingBox_VertexShader");
		void *DebugDrawBoundingBoxPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel60.DebugDrawBoundingBox_PixelShader");
		SIZE_T DebugDrawBoundingBoxPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel60.DebugDrawBoundingBox_PixelShader");

		D3D12_INPUT_ELEMENT_DESC InputElementDesc;
		InputElementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		InputElementDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
		InputElementDesc.InputSlot = 0;
		InputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc.InstanceDataStepRate = 0;
		InputElementDesc.SemanticIndex = 0;
		InputElementDesc.SemanticName = "POSITION";

		D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
		ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
		GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
		GraphicsPipelineStateDesc.InputLayout.NumElements = 1;
		GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = &InputElementDesc;
		GraphicsPipelineStateDesc.NodeMask = 0;
		GraphicsPipelineStateDesc.NumRenderTargets = 1;
		GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
		GraphicsPipelineStateDesc.PS.BytecodeLength = DebugDrawBoundingBoxPixelShaderByteCodeLength;
		GraphicsPipelineStateDesc.PS.pShaderBytecode = DebugDrawBoundingBoxPixelShaderByteCodeData;
		GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
		GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		GraphicsPipelineStateDesc.SampleDesc.Count = 1;
		GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
		GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		GraphicsPipelineStateDesc.VS.BytecodeLength = DebugDrawBoundingBoxVertexShaderByteCodeLength;
		GraphicsPipelineStateDesc.VS.pShaderBytecode = DebugDrawBoundingBoxVertexShaderByteCodeData;

		SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DebugDrawBoundingBoxPipelineState)));
	}

	// ===============================================================================================================

	clusterizationSubSystem.PreComputeClustersPlanes();
}

void RenderSystem::ShutdownSystem()
{
	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;

	if (FrameSyncFences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		SAFE_DX(FrameSyncFences[CurrentFrameIndex]->SetEventOnCompletion(1, FrameSyncEvent));
		DWORD WaitResult = WaitForSingleObject(FrameSyncEvent, INFINITE);
	}

	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	{
		delete renderMesh;
	}

	RenderMeshDestructionQueue.Clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	{
		delete renderMaterial;
	}

	RenderMaterialDestructionQueue.Clear();

	for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	{
		delete renderTexture;
	}

	RenderTextureDestructionQueue.Clear();

	BOOL Result;

	Result = CloseHandle(FrameSyncEvent);
}

void RenderSystem::TickSystem(float DeltaTime)
{
	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	Camera& camera = gameFramework.GetCamera();

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();

	XMMATRIX ShadowViewMatrices[4], ShadowProjMatrices[4], ShadowViewProjMatrices[4];

	ShadowViewMatrices[0] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 10.0f, CameraLocation.y + 10.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[1] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 20.0f, CameraLocation.y + 20.0f, CameraLocation.z - 20.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[2] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 50.0f, CameraLocation.y + 50.0f, CameraLocation.z - 50.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[3] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 100.0f, CameraLocation.y + 100.0f, CameraLocation.z - 100.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ShadowProjMatrices[0] = XMMatrixOrthographicLH(10.0f, 10.0f, 0.01f, 500.0f);
	ShadowProjMatrices[1] = XMMatrixOrthographicLH(20.0f, 20.0f, 0.01f, 500.0f);
	ShadowProjMatrices[2] = XMMatrixOrthographicLH(50.0f, 50.0f, 0.01f, 500.0f);
	ShadowProjMatrices[3] = XMMatrixOrthographicLH(100.0f, 100.0f, 0.01f, 500.0f);

	ShadowViewProjMatrices[0] = ShadowViewMatrices[0] * ShadowProjMatrices[0];
	ShadowViewProjMatrices[1] = ShadowViewMatrices[1] * ShadowProjMatrices[1];
	ShadowViewProjMatrices[2] = ShadowViewMatrices[2] * ShadowProjMatrices[2];
	ShadowViewProjMatrices[3] = ShadowViewMatrices[3] * ShadowProjMatrices[3];

	if (FrameSyncFences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		SAFE_DX(FrameSyncFences[CurrentFrameIndex]->SetEventOnCompletion(1, FrameSyncEvent));
		DWORD WaitResult = WaitForSingleObject(FrameSyncEvent, INFINITE);
	}

	SAFE_DX(FrameSyncFences[CurrentFrameIndex]->Signal(0));

	{
		OPTICK_EVENT("Occlusion Buffer Reprojection");

		D3D12_RESOURCE_DESC ResourceDesc = OcclusionBufferTexture->DXTexture->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

		UINT NumRows;
		UINT64 RowSizeInBytes, TotalBytes;

		Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

		float *OcclusionBufferData = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetOcclusionBufferData();

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = TotalBytes;

		WrittenRange.Begin = 0;
		WrittenRange.End = 0;

		void *MappedData;

		OcclusionBufferTextureReadback[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &MappedData);

		for (UINT i = 0; i < NumRows; i++)
		{
			memcpy((BYTE*)OcclusionBufferData + i * RowSizeInBytes, (BYTE*)MappedData + i * PlacedSubResourceFootPrint.Footprint.RowPitch, RowSizeInBytes);
		}

		OcclusionBufferTextureReadback[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().ReProjectOcclusionBuffer(ViewProjMatrix, CurrentFrameIndex);
	}

	RenderScene& renderScene = gameFramework.GetWorld().GetRenderScene();

	SAFE_DX(GraphicsCommandAllocators[CurrentFrameIndex]->Reset());
	SAFE_DX(GraphicsCommandList->Reset(GraphicsCommandAllocators[CurrentFrameIndex], nullptr));

	FrameDescriptorHeap& FrameResourcesDescriptorHeap = *FrameResourcesDescriptorHeaps[CurrentFrameIndex];
	FrameDescriptorHeap& FrameSamplersDescriptorHeap = *FrameSamplersDescriptorHeaps[CurrentFrameIndex];

	FrameResourcesDescriptorHeaps[CurrentFrameIndex]->Reset();
	FrameSamplersDescriptorHeaps[CurrentFrameIndex]->Reset();

	ID3D12DescriptorHeap *DescriptorHeaps[2] = { FrameResourcesDescriptorHeap.GetDescriptorHeap(), FrameSamplersDescriptorHeaps[CurrentFrameIndex]->GetDescriptorHeap() };

	GraphicsCommandList->SetDescriptorHeaps(2, DescriptorHeaps);
	GraphicsCommandList->SetGraphicsRootSignature(GraphicsRootSignature);
	GraphicsCommandList->SetComputeRootSignature(ComputeRootSignature);

	// ===============================================================================================================

	SetTextureState(GBufferTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	SetTextureState(GBufferTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	SetTextureState(GBufferTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	// ===============================================================================================================

	DynamicArray<StaticMeshComponent*> VisibleStaticMeshComponents;
	size_t VisibleStaticMeshComponentsCount;

	{
		DynamicArray<StaticMeshComponent*>& AllStaticMeshComponents = renderScene.GetStaticMeshComponents();
				
		{
			OPTICK_EVENT("Main Camera Objects Culling")

			VisibleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);			
		}

		VisibleStaticMeshComponentsCount = VisibleStaticMeshComponents.GetLength();

		void *ConstantBufferData;
		SIZE_T ConstantBufferOffset = 0;

		{
			OPTICK_EVENT("Main Camera Objects Constants Filling")

			D3D12_RANGE ReadRange, WrittenRange;
			ReadRange.Begin = 0;
			ReadRange.End = 0;

			SAFE_DX(CPUCameraConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

			CameraConstantBuffer& ConstantBuffer = *((CameraConstantBuffer*)ConstantBufferData);

			XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

			ConstantBuffer.ViewProjMatrix = ViewProjMatrix;
			ConstantBuffer.InvViewProjMatrix = InvViewProjMatrix;
			ConstantBuffer.CameraWorldPosition = CameraLocation;
			ConstantBuffer.NearZ = 1000.0f;
			ConstantBuffer.FarZ = 0.01f;

			WrittenRange.Begin = 0;
			WrittenRange.End = 256;

			CPUCameraConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

			ReadRange.Begin = 0;
			ReadRange.End = 0;

			SAFE_DX(CPURenderTargetConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

			*(float*)(ConstantBufferData) = 1280.0f;
			*(float*)((BYTE*)ConstantBufferData + 4) = 720.0f;

			WrittenRange.Begin = 0;
			WrittenRange.End = 256;

			CPURenderTargetConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

			ReadRange.Begin = 0;
			ReadRange.End = 0;

			SAFE_DX(CPUGBufferOpaquePassObjectsConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

			for (size_t k = 0; k < VisibleStaticMeshComponentsCount; k++)
			{
				GBufferOpaquePassConstantBuffer& ConstantBuffer = *((GBufferOpaquePassConstantBuffer*)((BYTE*)ConstantBufferData + ConstantBufferOffset));

				XMMATRIX WorldMatrix = VisibleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
				//XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;

				XMFLOAT3X4 VectorTransformMatrix;

				float Determinant =
					WorldMatrix.m[0][0] * (WorldMatrix.m[1][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[1][2]) -
					WorldMatrix.m[1][0] * (WorldMatrix.m[0][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[0][2]) +
					WorldMatrix.m[2][0] * (WorldMatrix.m[0][1] * WorldMatrix.m[1][2] - WorldMatrix.m[1][1] * WorldMatrix.m[0][2]);

				VectorTransformMatrix.m[0][0] = (WorldMatrix.m[1][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[1][2]) / Determinant;
				VectorTransformMatrix.m[1][0] = -(WorldMatrix.m[0][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[0][2]) / Determinant;
				VectorTransformMatrix.m[2][0] = (WorldMatrix.m[0][1] * WorldMatrix.m[1][2] - WorldMatrix.m[1][1] * WorldMatrix.m[0][2]) / Determinant;

				VectorTransformMatrix.m[0][1] = -(WorldMatrix.m[1][0] * WorldMatrix.m[2][2] - WorldMatrix.m[2][0] * WorldMatrix.m[1][2]) / Determinant;
				VectorTransformMatrix.m[1][1] = (WorldMatrix.m[0][0] * WorldMatrix.m[2][2] - WorldMatrix.m[2][0] * WorldMatrix.m[0][2]) / Determinant;
				VectorTransformMatrix.m[2][1] = -(WorldMatrix.m[0][0] * WorldMatrix.m[1][0] - WorldMatrix.m[0][2] * WorldMatrix.m[1][2]) / Determinant;

				VectorTransformMatrix.m[0][2] = (WorldMatrix.m[1][0] * WorldMatrix.m[2][1] - WorldMatrix.m[2][0] * WorldMatrix.m[1][1]) / Determinant;
				VectorTransformMatrix.m[1][2] = -(WorldMatrix.m[0][0] * WorldMatrix.m[2][1] - WorldMatrix.m[2][0] * WorldMatrix.m[0][1]) / Determinant;
				VectorTransformMatrix.m[2][2] = (WorldMatrix.m[0][0] * WorldMatrix.m[1][1] - WorldMatrix.m[1][0] * WorldMatrix.m[0][1]) / Determinant;

				VectorTransformMatrix.m[0][3] = 0.0f;
				VectorTransformMatrix.m[1][3] = 0.0f;
				VectorTransformMatrix.m[2][3] = 0.0f;

				//ConstantBuffer.WVPMatrix = WVPMatrix;
				ConstantBuffer.WorldMatrix = WorldMatrix;
				ConstantBuffer.VectorTransformMatrix = VectorTransformMatrix;

				ConstantBufferOffset += 256;
			}

			WrittenRange.Begin = 0;
			WrittenRange.End = ConstantBufferOffset;

			CPUGBufferOpaquePassObjectsConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);
		}

		SetBufferState(GPUCameraConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		SetBufferState(GPUGBufferOpaquePassObjectsConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		SetBufferState(GPURenderTargetConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		ApplyPendingBarriers();

		GraphicsCommandList->CopyBufferRegion(GPUCameraConstantBuffer->DXBuffer, 0, CPUCameraConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256);
		GraphicsCommandList->CopyBufferRegion(GPURenderTargetConstantBuffer->DXBuffer, 0, CPURenderTargetConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256);
		GraphicsCommandList->CopyBufferRegion(GPUGBufferOpaquePassObjectsConstantBuffer->DXBuffer, 0, CPUGBufferOpaquePassObjectsConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, ConstantBufferOffset);

		SetBufferState(GPUCameraConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		SetBufferState(GPUGBufferOpaquePassObjectsConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		SetBufferState(GPURenderTargetConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		GraphicsCommandList->OMSetRenderTargets(3, GBufferTexturesRTVs, TRUE, &DepthBufferTextureDSV);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		GraphicsCommandList->ClearRenderTargetView(GBufferTexturesRTVs[0], ClearColor, 0, nullptr);
		GraphicsCommandList->ClearRenderTargetView(GBufferTexturesRTVs[1], ClearColor, 0, nullptr);
		GraphicsCommandList->ClearRenderTargetView(GBufferTexturesRTVs[2], ClearColor, 0, nullptr);
		GraphicsCommandList->ClearDepthStencilView(DepthBufferTextureDSV, D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);

		DescriptorTable TextureSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(1);
		TextureSamplerTable.SetSampler(0, TextureSampler);
		TextureSamplerTable.UpdateTable(Device);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SAMPLERS, TextureSamplerTable);
		
		{
			OPTICK_EVENT("Main Camera Draw Calls")

			for (size_t k = 0; k < VisibleStaticMeshComponentsCount; k++)
			{
				StaticMeshComponent *staticMeshComponent = VisibleStaticMeshComponents[k];

				RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
				RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
				MaterialResource *material = staticMeshComponent->GetMaterial();
				RenderTexture *renderTexture0 = material->GetTexture(0)->GetRenderTexture();
				RenderTexture *renderTexture1 = material->GetTexture(1)->GetRenderTexture();

				DescriptorTable VertexShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(2);
				DescriptorTable PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(2);

				VertexShaderResourcesTable.SetConstantBuffer(0, CameraConstantBufferCBV);
				VertexShaderResourcesTable.SetConstantBuffer(1, GBufferOpaquePassObjectsConstantBufferCBVs[k]);

				PixelShaderResourcesTable.SetTexture(0, renderTexture0->TextureSRV);
				PixelShaderResourcesTable.SetTexture(1, renderTexture1->TextureSRV);

				VertexShaderResourcesTable.UpdateTable(Device);
				PixelShaderResourcesTable.UpdateTable(Device);

				D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[3];
				VertexBufferViews[0].BufferLocation = renderMesh->VertexBufferAddresses[0];
				VertexBufferViews[0].SizeInBytes = sizeof(XMFLOAT3) * staticMeshComponent->GetStaticMesh()->GetVertexCount();
				VertexBufferViews[0].StrideInBytes = sizeof(XMFLOAT3);
				VertexBufferViews[1].BufferLocation = renderMesh->VertexBufferAddresses[1];
				VertexBufferViews[1].SizeInBytes = sizeof(XMFLOAT2) * staticMeshComponent->GetStaticMesh()->GetVertexCount();
				VertexBufferViews[1].StrideInBytes = sizeof(XMFLOAT2);
				VertexBufferViews[2].BufferLocation = renderMesh->VertexBufferAddresses[2];
				VertexBufferViews[2].SizeInBytes = 3 * sizeof(XMFLOAT3) * staticMeshComponent->GetStaticMesh()->GetVertexCount();
				VertexBufferViews[2].StrideInBytes = 3 * sizeof(XMFLOAT3);

				D3D12_INDEX_BUFFER_VIEW IndexBufferView;
				IndexBufferView.BufferLocation = renderMesh->IndexBufferAddress;
				IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
				IndexBufferView.SizeInBytes = sizeof(WORD) * staticMeshComponent->GetStaticMesh()->GetIndexCount();

				GraphicsCommandList->IASetVertexBuffers(0, 3, VertexBufferViews);
				GraphicsCommandList->IASetIndexBuffer(&IndexBufferView);

				GraphicsCommandList->SetPipelineState(renderMaterial->GBufferOpaquePassPipelineState);

				GraphicsCommandList->SetGraphicsRootDescriptorTable(VERTEX_SHADER_CONSTANT_BUFFERS, VertexShaderResourcesTable);
				GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

				XMFLOAT3 ObjectLocation = staticMeshComponent->GetTransformComponent()->GetLocation();

				float Distance = sqrtf(powf(ObjectLocation.x - CameraLocation.x, 2.0f) + powf(ObjectLocation.y - CameraLocation.y, 2.0f) + powf(ObjectLocation.z - CameraLocation.z, 2.0f));

				UINT LODIndex;

				if (Distance < 20.0f)
					LODIndex = 0;
				else if (Distance < 50.0f)
					LODIndex = 1;
				else if (Distance < 100.0f)
					LODIndex = 2;
				else if (Distance < 250.0f)
					LODIndex = 3;
				else
					LODIndex = 4;

				UINT IndexCount = staticMeshComponent->GetStaticMesh()->GetIndexCount(LODIndex);
				UINT VertexOffset = staticMeshComponent->GetStaticMesh()->GetVertexOffset(LODIndex);
				UINT IndexOffset = staticMeshComponent->GetStaticMesh()->GetIndexOffset(LODIndex);

				GraphicsCommandList->DrawIndexedInstanced(IndexCount, 1, IndexOffset, VertexOffset, 0);
			}
		}
	}

	// ===============================================================================================================

	SetTextureState(DepthBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	SetTextureState(ResolvedDepthBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST);

	// ===============================================================================================================

	{
		ApplyPendingBarriers();

		COMRCPtr<ID3D12GraphicsCommandList1> GraphicsCommandList1;

		SAFE_DX(GraphicsCommandList->QueryInterface<ID3D12GraphicsCommandList1>(&GraphicsCommandList1));

		GraphicsCommandList1->ResolveSubresourceRegion(ResolvedDepthBufferTexture->DXTexture, 0, 0, 0, DepthBufferTexture->DXTexture, 0, nullptr, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT, D3D12_RESOLVE_MODE::D3D12_RESOLVE_MODE_MAX);
	}

	// ===============================================================================================================

	SetTextureState(ResolvedDepthBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// ===============================================================================================================

	{
		SetTextureState(OcclusionBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &OcclusionBufferTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = 144.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = 256.0f;

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = 144;
		ScissorRect.left = 0;
		ScissorRect.right = 256;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		GraphicsCommandList->DiscardResource(OcclusionBufferTexture->DXTexture, nullptr);

		DescriptorTable MinSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(1);
		MinSamplerTable.SetSampler(0, MinSampler);
		MinSamplerTable.UpdateTable(Device);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SAMPLERS, MinSamplerTable);

		DescriptorTable ShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		ShaderResourcesTable.SetTexture(0, ResolvedDepthBufferTextureSRV);
		ShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(OcclusionBufferPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, ShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

		SetTextureState(OcclusionBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

		ApplyPendingBarriers();

		D3D12_RESOURCE_DESC ResourceDesc = OcclusionBufferTexture->DXTexture->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

		UINT NumRows;
		UINT64 RowSizeInBytes, TotalBytes;

		Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

		D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

		SourceTextureCopyLocation.pResource = OcclusionBufferTexture->DXTexture;
		SourceTextureCopyLocation.SubresourceIndex = 0;
		SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		DestTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
		DestTextureCopyLocation.pResource = OcclusionBufferTextureReadback[CurrentFrameIndex]->DXBuffer;
		DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		GraphicsCommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);
	}

	// ===============================================================================================================

	SetTextureState(CascadedShadowMapTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);
	SetTextureState(CascadedShadowMapTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);
	SetTextureState(CascadedShadowMapTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);
	SetTextureState(CascadedShadowMapTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// ===============================================================================================================

	{
		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUShadowMapCameraConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

		for (int i = 0; i < 4; i++)
		{
			CameraConstantBuffer& ConstantBuffer = *((CameraConstantBuffer*)((BYTE*)ConstantBufferData + i * 256));

			ConstantBuffer.ViewProjMatrix = ShadowViewProjMatrices[i];
		}

		WrittenRange.Begin = 0;
		WrittenRange.End = 256 * 4;

		CPUShadowMapCameraConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		SetBufferState(GPUShadowMapCameraConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		ApplyPendingBarriers();

		GraphicsCommandList->CopyBufferRegion(GPUShadowMapCameraConstantBuffer->DXBuffer, 0, CPUShadowMapCameraConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256 * 4);

		SetBufferState(GPUShadowMapCameraConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		ApplyPendingBarriers();

		for (int i = 0; i < 4; i++)
		{
			SIZE_T ConstantBufferOffset = 0;

			DynamicArray<StaticMeshComponent*>& AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
			DynamicArray<StaticMeshComponent*> VisibleStaticMeshComponents;

			{
				OPTICK_EVENT("Shadow Frustum Objects Culling");

				VisibleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i], false);
			}

			size_t VisibleStaticMeshComponentsCount = VisibleStaticMeshComponents.GetLength();

			D3D12_RANGE ReadRange, WrittenRange;
			ReadRange.Begin = 0;
			ReadRange.End = 0;

			void *ConstantBufferData;

			{
				OPTICK_EVENT("Shadow Frustum Objects Constants Filling");

				SAFE_DX(CPUShadowMapPassObjectsConstantBuffers[i][CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

				for (size_t k = 0; k < VisibleStaticMeshComponentsCount; k++)
				{
					XMMATRIX WorldMatrix = VisibleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
					//XMMATRIX WVPMatrix = WorldMatrix * ShadowViewProjMatrices[i];

					ShadowMapPassConstantBuffer& ConstantBuffer = *((ShadowMapPassConstantBuffer*)((BYTE*)ConstantBufferData + ConstantBufferOffset));

					//ConstantBuffer.WVPMatrix = WVPMatrix;
					ConstantBuffer.WorldMatrix = WorldMatrix;

					ConstantBufferOffset += 256;
				}

				WrittenRange.Begin = 0;
				WrittenRange.End = ConstantBufferOffset;

				CPUShadowMapPassObjectsConstantBuffers[i][CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);
			}

			SetBufferState(GPUShadowMapPassObjectsConstantBuffers[i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

			ApplyPendingBarriers();

			GraphicsCommandList->CopyBufferRegion(GPUShadowMapPassObjectsConstantBuffers[i]->DXBuffer, 0, CPUShadowMapPassObjectsConstantBuffers[i][CurrentFrameIndex]->DXBuffer, 0, ConstantBufferOffset);

			SetBufferState(GPUShadowMapPassObjectsConstantBuffers[i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			ApplyPendingBarriers();

			GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			GraphicsCommandList->OMSetRenderTargets(0, nullptr, TRUE, &CascadedShadowMapTexturesDSVs[i]);

			D3D12_VIEWPORT Viewport;
			Viewport.Height = 2048.0f;
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = 2048.0f;

			GraphicsCommandList->RSSetViewports(1, &Viewport);

			D3D12_RECT ScissorRect;
			ScissorRect.bottom = 2048;
			ScissorRect.left = 0;
			ScissorRect.right = 2048;
			ScissorRect.top = 0;

			GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

			GraphicsCommandList->ClearDepthStencilView(CascadedShadowMapTexturesDSVs[i], D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			{
				OPTICK_EVENT("Shadow Map Draw Calls")

				for (size_t k = 0; k < VisibleStaticMeshComponentsCount; k++)
				{
					StaticMeshComponent *staticMeshComponent = VisibleStaticMeshComponents[k];

					RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
					RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
					RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
					RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

					DescriptorTable VertexShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(2);

					VertexShaderResourcesTable.SetConstantBuffer(0, ShadowMapCameraConstantBufferCBVs[i]);
					VertexShaderResourcesTable.SetConstantBuffer(1, ShadowMapPassObjectsConstantBufferCBVs[i][k]);

					VertexShaderResourcesTable.UpdateTable(Device);

					D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
					VertexBufferView.BufferLocation = renderMesh->VertexBufferAddresses[0];
					VertexBufferView.SizeInBytes = sizeof(XMFLOAT3) * staticMeshComponent->GetStaticMesh()->GetVertexCount();
					VertexBufferView.StrideInBytes = sizeof(XMFLOAT3);

					D3D12_INDEX_BUFFER_VIEW IndexBufferView;
					IndexBufferView.BufferLocation = renderMesh->IndexBufferAddress;
					IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
					IndexBufferView.SizeInBytes = sizeof(WORD) * staticMeshComponent->GetStaticMesh()->GetIndexCount();

					GraphicsCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
					GraphicsCommandList->IASetIndexBuffer(&IndexBufferView);

					GraphicsCommandList->SetPipelineState(renderMaterial->ShadowMapPassPipelineState);

					GraphicsCommandList->SetGraphicsRootDescriptorTable(VERTEX_SHADER_CONSTANT_BUFFERS, VertexShaderResourcesTable);

					XMFLOAT3 ObjectLocation = staticMeshComponent->GetTransformComponent()->GetLocation();

					float Distance = sqrtf(powf(ObjectLocation.x - CameraLocation.x, 2.0f) + powf(ObjectLocation.y - CameraLocation.y, 2.0f) + powf(ObjectLocation.z - CameraLocation.z, 2.0f));

					UINT LODIndex;

					if (Distance < 20.0f)
						LODIndex = 0;
					else if (Distance < 50.0f)
						LODIndex = 1;
					else if (Distance < 100.0f)
						LODIndex = 2;
					else if (Distance < 250.0f)
						LODIndex = 3;
					else
						LODIndex = 4;

					UINT IndexCount = staticMeshComponent->GetStaticMesh()->GetIndexCount(LODIndex);
					UINT VertexOffset = staticMeshComponent->GetStaticMesh()->GetVertexOffset(LODIndex);
					UINT IndexOffset = staticMeshComponent->GetStaticMesh()->GetIndexOffset(LODIndex);

					GraphicsCommandList->DrawIndexedInstanced(IndexCount, 1, IndexOffset, VertexOffset, 0);
				}
			}
		}
	}

	// ===============================================================================================================

	SetTextureState(ShadowMaskTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	SetTextureState(CascadedShadowMapTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(CascadedShadowMapTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(CascadedShadowMapTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(CascadedShadowMapTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// ===============================================================================================================

	{
		XMMATRIX ReProjMatrices[4];
		ReProjMatrices[0] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[0];
		ReProjMatrices[1] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[1];
		ReProjMatrices[2] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[2];
		ReProjMatrices[3] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[3];

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUShadowResolveConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

		ShadowResolveConstantBuffer& ConstantBuffer = *((ShadowResolveConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.ReProjMatrices[0] = ReProjMatrices[0];
		ConstantBuffer.ReProjMatrices[1] = ReProjMatrices[1];
		ConstantBuffer.ReProjMatrices[2] = ReProjMatrices[2];
		ConstantBuffer.ReProjMatrices[3] = ReProjMatrices[3];

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUShadowResolveConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		SetBufferState(GPUShadowResolveConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		ApplyPendingBarriers();

		GraphicsCommandList->CopyBufferRegion(GPUShadowResolveConstantBuffer->DXBuffer, 0, CPUShadowResolveConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256);

		SetBufferState(GPUShadowResolveConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &ShadowMaskTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		GraphicsCommandList->DiscardResource(ShadowMaskTexture->DXTexture, nullptr);

		DescriptorTable MinSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(1);
		MinSamplerTable.SetSampler(0, ShadowMapSampler);
		MinSamplerTable.UpdateTable(Device);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SAMPLERS, MinSamplerTable);
		
		DescriptorTable PixelShaderConstantBuffersTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		DescriptorTable PixelShaderShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(5);

		PixelShaderConstantBuffersTable.SetConstantBuffer(0, ShadowResolveConstantBufferCBV);
		PixelShaderShaderResourcesTable.SetTexture(0, ResolvedDepthBufferTextureSRV);
		PixelShaderShaderResourcesTable.SetTexture(1, CascadedShadowMapTexturesSRVs[0]);
		PixelShaderShaderResourcesTable.SetTexture(2, CascadedShadowMapTexturesSRVs[1]);
		PixelShaderShaderResourcesTable.SetTexture(3, CascadedShadowMapTexturesSRVs[2]);
		PixelShaderShaderResourcesTable.SetTexture(4, CascadedShadowMapTexturesSRVs[3]);

		PixelShaderConstantBuffersTable.UpdateTable(Device);
		PixelShaderShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(ShadowResolvePipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_CONSTANT_BUFFERS, PixelShaderConstantBuffersTable);
		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	SetTextureState(GBufferTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(GBufferTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(GBufferTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(ShadowMaskTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(HDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	// ===============================================================================================================

	{
		DynamicArray<PointLight> PointLights;

		{
			OPTICK_EVENT("Light Culling and Clusterization")

			DynamicArray<PointLightComponent*>& AllPointLightComponents = renderScene.GetPointLightComponents();
			DynamicArray<PointLightComponent*> VisiblePointLightComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

			Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().ClusterizeLights(VisiblePointLightComponents, ViewMatrix);

			for (PointLightComponent *pointLightComponent : VisiblePointLightComponents)
			{
				PointLight pointLight;
				pointLight.Brightness = pointLightComponent->GetBrightness();
				pointLight.Color = pointLightComponent->GetColor();
				pointLight.Position = pointLightComponent->GetTransformComponent()->GetLocation();
				pointLight.Radius = pointLightComponent->GetRadius();

				PointLights.Add(pointLight);
			}
		}

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPULightingConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));
		
		*(XMFLOAT3*)(ConstantBufferData) = XMFLOAT3(0.0f, 0.0f, 0.0f);

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPULightingConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		ReadRange.Begin = 0;
		ReadRange.End = 0;

		SAFE_DX(CPUClusteredShadingConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

		*(UINT*)((BYTE*)ConstantBufferData) = ClusterizationSubSystem::CLUSTERS_COUNT_X;
		*(UINT*)((BYTE*)ConstantBufferData + 4) = ClusterizationSubSystem::CLUSTERS_COUNT_Y;
		*(UINT*)((BYTE*)ConstantBufferData + 8) = ClusterizationSubSystem::CLUSTERS_COUNT_Z;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUClusteredShadingConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		void *DynamicBufferData;

		SAFE_DX(CPULightClustersBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

		WrittenRange.Begin = 0;
		WrittenRange.End = 32 * 18 * 24 * 2 * sizeof(uint32_t);

		CPULightClustersBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CPULightIndicesBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetLightIndicesData(), Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t));

		WrittenRange.Begin = 0;
		WrittenRange.End = Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t);

		CPULightIndicesBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CPUPointLightsBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, PointLights.GetData(), PointLights.GetLength() * sizeof(PointLight));

		WrittenRange.Begin = 0;
		WrittenRange.End = PointLights.GetLength() * sizeof(PointLight);

		CPUPointLightsBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		SetBufferState(GPULightingConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		SetBufferState(GPUClusteredShadingConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		SetBufferState(GPULightClustersBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		SetBufferState(GPULightIndicesBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		SetBufferState(GPUPointLightsBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		ApplyPendingBarriers();

		GraphicsCommandList->CopyBufferRegion(GPULightingConstantBuffer->DXBuffer, 0, CPULightingConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256);
		GraphicsCommandList->CopyBufferRegion(GPUClusteredShadingConstantBuffer->DXBuffer, 0, CPUClusteredShadingConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256);
		GraphicsCommandList->CopyBufferRegion(GPULightClustersBuffer->DXBuffer, 0, CPULightClustersBuffers[CurrentFrameIndex]->DXBuffer, 0, ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * sizeof(LightCluster));
		GraphicsCommandList->CopyBufferRegion(GPULightIndicesBuffer->DXBuffer, 0, CPULightIndicesBuffers[CurrentFrameIndex]->DXBuffer, 0, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t));
		GraphicsCommandList->CopyBufferRegion(GPUPointLightsBuffer->DXBuffer, 0, CPUPointLightsBuffers[CurrentFrameIndex]->DXBuffer, 0, PointLights.GetLength() * sizeof(PointLight));

		SetBufferState(GPULightingConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		SetBufferState(GPUClusteredShadingConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		SetBufferState(GPULightClustersBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		SetBufferState(GPULightIndicesBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		SetBufferState(GPUPointLightsBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		GraphicsCommandList->DiscardResource(HDRSceneColorTexture->DXTexture, nullptr);

		DescriptorTable PixelShaderConstantBuffersTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(4);
		DescriptorTable PixelShaderShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(8);

		PixelShaderConstantBuffersTable.SetConstantBuffer(0, CameraConstantBufferCBV);
		PixelShaderConstantBuffersTable.SetConstantBuffer(1, LightingConstantBufferCBV);
		PixelShaderConstantBuffersTable.SetConstantBuffer(2, RenderTargetConstantBufferCBV);
		PixelShaderConstantBuffersTable.SetConstantBuffer(3, ClusteredShadingConstantBufferCBV);
		PixelShaderShaderResourcesTable.SetTexture(0, GBufferTexturesSRVs[0]);
		PixelShaderShaderResourcesTable.SetTexture(1, GBufferTexturesSRVs[1]);
		PixelShaderShaderResourcesTable.SetTexture(2, GBufferTexturesSRVs[2]);
		PixelShaderShaderResourcesTable.SetTexture(3, DepthBufferTextureSRV);
		PixelShaderShaderResourcesTable.SetTexture(4, ShadowMaskTextureSRV);
		PixelShaderShaderResourcesTable.SetTexture(5, LightClustersBufferSRV);
		PixelShaderShaderResourcesTable.SetTexture(6, LightIndicesBufferSRV);
		PixelShaderShaderResourcesTable.SetTexture(7, PointLightsBufferSRV);

		PixelShaderConstantBuffersTable.UpdateTable(Device);
		PixelShaderShaderResourcesTable.UpdateTable(Device);		

		GraphicsCommandList->SetPipelineState(DeferredLightingPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_CONSTANT_BUFFERS, PixelShaderConstantBuffersTable);
		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	{
		XMMATRIX SkyWorldMatrix = XMMatrixScaling(900.0f, 900.0f, 900.0f) * XMMatrixTranslation(CameraLocation.x, CameraLocation.y, CameraLocation.z);
		XMMATRIX SkyWVPMatrix = SkyWorldMatrix * ViewProjMatrix;

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUSkyConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

		SkyConstantBuffer& skyConstantBuffer = *((SkyConstantBuffer*)((BYTE*)ConstantBufferData));

		skyConstantBuffer.WVPMatrix = SkyWVPMatrix;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUSkyConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);

		SAFE_DX(CPUSunConstantBuffers[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

		SunConstantBuffer& sunConstantBuffer = *((SunConstantBuffer*)((BYTE*)ConstantBufferData));

		sunConstantBuffer.ViewMatrix = ViewMatrix;
		sunConstantBuffer.ProjMatrix = ProjMatrix;
		sunConstantBuffer.SunPosition = SunPosition;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUSunConstantBuffers[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		SetBufferState(GPUSkyConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		SetBufferState(GPUSunConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		ApplyPendingBarriers();

		GraphicsCommandList->CopyBufferRegion(GPUSkyConstantBuffer->DXBuffer, 0, CPUSkyConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256);
		GraphicsCommandList->CopyBufferRegion(GPUSunConstantBuffer->DXBuffer, 0, CPUSunConstantBuffers[CurrentFrameIndex]->DXBuffer, 0, 256);

		SetBufferState(GPUSkyConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		SetBufferState(GPUSunConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		DescriptorTable FogPixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);

		FogPixelShaderResourcesTable.SetTexture(0, DepthBufferTextureSRV);

		FogPixelShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(FogPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, FogPixelShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

		SetTextureState(DepthBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		GraphicsCommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, &DepthBufferTextureDSV);

		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		DescriptorTable TextureSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(1);
		TextureSamplerTable.SetSampler(0, TextureSampler);
		TextureSamplerTable.UpdateTable(Device);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SAMPLERS, TextureSamplerTable);
		
		DescriptorTable VertexShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		DescriptorTable PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);

		VertexShaderResourcesTable.SetConstantBuffer(0, SkyConstantBufferCBV);

		PixelShaderResourcesTable.SetTexture(0, SkyTextureSRV);

		VertexShaderResourcesTable.UpdateTable(Device);
		PixelShaderResourcesTable.UpdateTable(Device);

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		VertexBufferView.BufferLocation = SkyVertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * (1 + 25 * 100 + 1);
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		IndexBufferView.BufferLocation = SkyIndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * (300 + 24 * 600 + 300);

		GraphicsCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		GraphicsCommandList->IASetIndexBuffer(&IndexBufferView);

		GraphicsCommandList->SetPipelineState(SkyPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(VERTEX_SHADER_CONSTANT_BUFFERS, VertexShaderResourcesTable);
		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DrawIndexedInstanced(300 + 24 * 600 + 300, 1, 0, 0, 0);

		VertexShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);

		VertexShaderResourcesTable.SetConstantBuffer(0, SunConstantBufferCBV);

		PixelShaderResourcesTable.SetTexture(0, SunTextureSRV);

		VertexShaderResourcesTable.UpdateTable(Device);
		PixelShaderResourcesTable.UpdateTable(Device);

		VertexBufferView.BufferLocation = SunVertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * 4;
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		IndexBufferView.BufferLocation = SunIndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * 6;

		GraphicsCommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		GraphicsCommandList->IASetIndexBuffer(&IndexBufferView);

		GraphicsCommandList->SetPipelineState(SunPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(VERTEX_SHADER_CONSTANT_BUFFERS, VertexShaderResourcesTable);
		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	// ===============================================================================================================

	SetTextureState(HDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	SetTextureState(ResolvedHDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST);

	// ===============================================================================================================

	{
		ApplyPendingBarriers();

		GraphicsCommandList->ResolveSubresource(ResolvedHDRSceneColorTexture->DXTexture, 0, HDRSceneColorTexture->DXTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	// ===============================================================================================================

	SetTextureState(ResolvedHDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	SetTextureState(SceneLuminanceTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	// ===============================================================================================================

	{
		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &SceneLuminanceTexturesRTVs[0], TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);
		
		DescriptorTable PixelShaderResourcesTable = FrameResourcesDescriptorHeaps[CurrentFrameIndex]->AllocateDescriptorTable(1);
		PixelShaderResourcesTable.SetTexture(0, ResolvedHDRSceneColorTextureSRV);
		PixelShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(LuminanceCalcPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DiscardResource(SceneLuminanceTextures[0]->DXTexture, nullptr);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

		int CurrentWidth = 640;
		int CurrentHeight = 360;

		for (int i = 1; i <= 11; i++)
		{
			SetTextureState(SceneLuminanceTextures[i - 1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			SetTextureState(SceneLuminanceTextures[i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

			ApplyPendingBarriers();

			GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			GraphicsCommandList->OMSetRenderTargets(1, &SceneLuminanceTexturesRTVs[i], TRUE, nullptr);

			Viewport.Height = FLOAT(CurrentHeight);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(CurrentWidth);

			GraphicsCommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = CurrentHeight;
			ScissorRect.left = 0;
			ScissorRect.right = CurrentWidth;
			ScissorRect.top = 0;

			GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

			CurrentWidth = CurrentWidth / 2 + (CurrentWidth & 1);
			CurrentHeight = CurrentHeight / 2 + (CurrentHeight & 1);

			DescriptorTable PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
			PixelShaderResourcesTable.SetTexture(0, SceneLuminanceTexturesSRVs[i - 1]);
			PixelShaderResourcesTable.UpdateTable(Device);

			GraphicsCommandList->SetPipelineState(LuminanceSumPipelineState);

			GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

			GraphicsCommandList->DiscardResource(SceneLuminanceTextures[i]->DXTexture, nullptr);

			GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
		}

		SetTextureState(SceneLuminanceTextures[11], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &AverageLuminanceTextureRTV, TRUE, nullptr);

		Viewport.Height = FLOAT(1);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(1);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = 1;
		ScissorRect.left = 0;
		ScissorRect.right = 1;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		PixelShaderResourcesTable.SetTexture(0, SceneLuminanceTexturesSRVs[11]);
		PixelShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(LuminanceAvgPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DiscardResource(AverageLuminanceTexture->DXTexture, nullptr);


		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	{
		SetTextureState(BloomTextures[0][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[0][0], TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		GraphicsCommandList->DiscardResource(BloomTextures[0][0]->DXTexture, nullptr);

		DescriptorTable PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(2);
		PixelShaderResourcesTable.SetTexture(0, ResolvedHDRSceneColorTextureSRV);
		PixelShaderResourcesTable.SetTexture(1, SceneLuminanceTexturesSRVs[0]);
		PixelShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(BrightPassPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

		SetTextureState(BloomTextures[0][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		SetTextureState(BloomTextures[1][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[1][0], TRUE, nullptr);

		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		GraphicsCommandList->DiscardResource(BloomTextures[1][0]->DXTexture, nullptr);

		PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		PixelShaderResourcesTable.SetTexture(0, BloomTexturesSRVs[0][0]);
		PixelShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(HorizontalBlurPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

		SetTextureState(BloomTextures[1][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		SetTextureState(BloomTextures[2][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][0], TRUE, nullptr);

		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		GraphicsCommandList->DiscardResource(BloomTextures[2][0]->DXTexture, nullptr);

		PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		PixelShaderResourcesTable.SetTexture(0, BloomTexturesSRVs[1][0]);
		PixelShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(VerticalBlurPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

		for (int i = 1; i < 7; i++)
		{
			SetTextureState(BloomTextures[0][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

			ApplyPendingBarriers();

			GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			GraphicsCommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[0][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			GraphicsCommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

			GraphicsCommandList->DiscardResource(BloomTextures[0][i]->DXTexture, nullptr);

			PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
			PixelShaderResourcesTable.SetTexture(0, BloomTexturesSRVs[0][i - 1]);
			PixelShaderResourcesTable.UpdateTable(Device);

			GraphicsCommandList->SetPipelineState(DownSamplePipelineState);

			GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

			GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

			SetTextureState(BloomTextures[0][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			SetTextureState(BloomTextures[1][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

			ApplyPendingBarriers();

			GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			GraphicsCommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[1][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			GraphicsCommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

			GraphicsCommandList->DiscardResource(BloomTextures[1][i]->DXTexture, nullptr);

			PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
			PixelShaderResourcesTable.SetTexture(0, BloomTexturesSRVs[0][i]);
			PixelShaderResourcesTable.UpdateTable(Device);

			GraphicsCommandList->SetPipelineState(HorizontalBlurPipelineState);

			GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

			GraphicsCommandList->DrawInstanced(4, 1, 0, 0);

			SetTextureState(BloomTextures[1][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			SetTextureState(BloomTextures[2][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

			ApplyPendingBarriers();

			GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			GraphicsCommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			GraphicsCommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

			GraphicsCommandList->DiscardResource(BloomTextures[2][i]->DXTexture, nullptr);

			PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
			PixelShaderResourcesTable.SetTexture(0, BloomTexturesSRVs[1][i]);
			PixelShaderResourcesTable.UpdateTable(Device);

			GraphicsCommandList->SetPipelineState(VerticalBlurPipelineState);

			GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

			GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
		}

		for (int i = 5; i >= 0; i--)
		{
			SetTextureState(BloomTextures[2][i + 1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			ApplyPendingBarriers();

			GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			GraphicsCommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			GraphicsCommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

			PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
			PixelShaderResourcesTable.SetTexture(0, BloomTexturesSRVs[2][i + 1]);
			PixelShaderResourcesTable.UpdateTable(Device);

			GraphicsCommandList->SetPipelineState(UpSampleWithAddBlendPipelineState);

			GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

			GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
		}
	}

	// ===============================================================================================================

	SetTextureState(BloomTextures[2][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	SetTextureState(ToneMappedImageTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	// ===============================================================================================================

	{
		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		GraphicsCommandList->OMSetRenderTargets(1, &ToneMappedImageTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);

		GraphicsCommandList->DiscardResource(ToneMappedImageTexture->DXTexture, nullptr);

		DescriptorTable PixelShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(2);
		PixelShaderResourcesTable.SetTexture(0, HDRSceneColorTextureSRV);
		PixelShaderResourcesTable.SetTexture(1, BloomTexturesSRVs[2][0]);
		PixelShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(HDRToneMappingPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, PixelShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	SetTextureState(ToneMappedImageTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	SwitchResourceState(BackBufferTextures[CurrentBackBufferIndex], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST);

	// ===============================================================================================================

	{
		ApplyPendingBarriers();

		GraphicsCommandList->ResolveSubresource(BackBufferTextures[CurrentBackBufferIndex], 0, ToneMappedImageTexture->DXTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	}

	// ===============================================================================================================

	SwitchResourceState(BackBufferTextures[CurrentBackBufferIndex], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	// ===============================================================================================================

	{
		ApplyPendingBarriers();

		GraphicsCommandList->OMSetRenderTargets(1, &BackBufferTexturesRTVs[CurrentBackBufferIndex], TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		GraphicsCommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		GraphicsCommandList->RSSetScissorRects(1, &ScissorRect);
	}

	// ===============================================================================================================

	if (DebugDrawBoundingBoxes)
	{
		GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_LINELIST);

		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		IndexBufferView.BufferLocation = BoundingBoxIndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = 24 * sizeof(WORD);

		GraphicsCommandList->IASetIndexBuffer(&IndexBufferView);

		GraphicsCommandList->SetPipelineState(DebugDrawBoundingBoxPipelineState);

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;
		SIZE_T ConstantBufferOffset = 0;

		SAFE_DX(CPUConstantBuffers3[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

		for (size_t k = 0; k < VisibleStaticMeshComponentsCount; k++)
		{
			StaticMeshComponent *staticMeshComponent = VisibleStaticMeshComponents[k];

			BoundingBoxComponent *boundingBoxComponent = staticMeshComponent->GetBoundingBoxComponent();

			XMFLOAT3 BBCenter = boundingBoxComponent->GetCenter();
			XMFLOAT3 BBHalfSize = boundingBoxComponent->GetHalfSize();

			XMVECTOR BoundingBoxVertices[8];
			BoundingBoxVertices[0] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
			BoundingBoxVertices[1] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
			BoundingBoxVertices[2] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
			BoundingBoxVertices[3] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
			BoundingBoxVertices[4] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);
			BoundingBoxVertices[5] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);
			BoundingBoxVertices[6] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);
			BoundingBoxVertices[7] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);

			XMMATRIX WorldMatrix = VisibleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
			XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;

			memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset, &WVPMatrix, sizeof(XMMATRIX));
			memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset + sizeof(XMMATRIX), BoundingBoxVertices, 8 * sizeof(XMVECTOR));

			ConstantBufferOffset += 256;
		}

		WrittenRange.Begin = 0;
		WrittenRange.End = ConstantBufferOffset;

		CPUConstantBuffers3[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		SetBufferState(GPUConstantBuffer3, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		ApplyPendingBarriers();

		GraphicsCommandList->CopyBufferRegion(GPUConstantBuffer3->DXBuffer, 0, CPUConstantBuffers3[CurrentFrameIndex]->DXBuffer, 0, ConstantBufferOffset);

		SetBufferState(GPUConstantBuffer3, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		ApplyPendingBarriers();

		for (size_t k = 0; k < VisibleStaticMeshComponentsCount; k++)
		{
			DescriptorTable ShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
			ShaderResourcesTable.SetTexture(0, ConstantBufferCBVs3[k]);
			ShaderResourcesTable.UpdateTable(Device);

			GraphicsCommandList->SetGraphicsRootDescriptorTable(VERTEX_SHADER_CONSTANT_BUFFERS, ShaderResourcesTable);

			GraphicsCommandList->DrawIndexedInstanced(24, 1, 0, 0, 0);
		}
	}

	// ===============================================================================================================

	if (DebugDrawOcclusionBuffer)
	{
		D3D12_RESOURCE_DESC ResourceDesc = DebugOcclusionBufferTexture->DXTexture->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

		UINT NumRows;
		UINT64 RowSizeInBytes, TotalBytes;

		Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

		float *OcclusionBufferData = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetReProjectedOcclusionBufferData();

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = TotalBytes;

		WrittenRange.Begin = 0;
		WrittenRange.End = 0;

		void *MappedData;

		DebugOcclusionBufferTextureUpload[CurrentFrameIndex]->DXBuffer->Map(0, &ReadRange, &MappedData);

		for (UINT i = 0; i < NumRows; i++)
		{
			memcpy((BYTE*)MappedData + i * PlacedSubResourceFootPrint.Footprint.RowPitch, (BYTE*)OcclusionBufferData + i * RowSizeInBytes, RowSizeInBytes);
		}

		DebugOcclusionBufferTextureUpload[CurrentFrameIndex]->DXBuffer->Unmap(0, &WrittenRange);

		D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

		SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
		SourceTextureCopyLocation.pResource = DebugOcclusionBufferTextureUpload[CurrentFrameIndex]->DXBuffer;
		SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		DestTextureCopyLocation.pResource = DebugOcclusionBufferTexture->DXTexture;
		DestTextureCopyLocation.SubresourceIndex = 0;
		DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		SetTextureState(DebugOcclusionBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

		ApplyPendingBarriers();

		GraphicsCommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

		SetTextureState(DebugOcclusionBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		ApplyPendingBarriers();

		GraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		DescriptorTable ShaderResourcesTable = FrameResourcesDescriptorHeap.AllocateDescriptorTable(1);
		ShaderResourcesTable.SetTexture(0, DebugOcclusionBufferTextureSRV);
		ShaderResourcesTable.UpdateTable(Device);

		GraphicsCommandList->SetPipelineState(DebugDrawOcclusionBufferPipelineState);

		GraphicsCommandList->SetGraphicsRootDescriptorTable(PIXEL_SHADER_SHADER_RESOURCES, ShaderResourcesTable);

		GraphicsCommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================
	
	SwitchResourceState(BackBufferTextures[CurrentBackBufferIndex], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);

	ApplyPendingBarriers();

	// ===============================================================================================================

	SAFE_DX(GraphicsCommandList->Close());

	GraphicsCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&GraphicsCommandList);

	SAFE_DX(SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));

	SAFE_DX(GraphicsCommandQueue->Signal(FrameSyncFences[CurrentFrameIndex], 1));

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

void RenderSystem::ApplyPendingBarriers()
{
	if (PendingResourceBarriersCount > 0)
	{
		GraphicsCommandList->ResourceBarrier(PendingResourceBarriersCount, PendingResourceBarriers);
		PendingResourceBarriersCount = 0;
	}
}

void RenderSystem::SwitchResourceState(ID3D12Resource* Resource, UINT SubResourceIndex, D3D12_RESOURCE_STATES OldState, D3D12_RESOURCE_STATES NewState)
{
	PendingResourceBarriers[PendingResourceBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	PendingResourceBarriers[PendingResourceBarriersCount].Transition.pResource = Resource;
	PendingResourceBarriers[PendingResourceBarriersCount].Transition.StateAfter = NewState;
	PendingResourceBarriers[PendingResourceBarriersCount].Transition.StateBefore = OldState;
	PendingResourceBarriers[PendingResourceBarriersCount].Transition.Subresource = SubResourceIndex;
	PendingResourceBarriers[PendingResourceBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	
	++PendingResourceBarriersCount;
}

void RenderSystem::SetBufferState(Pointer<Buffer>& BufferPtr, D3D12_RESOURCE_STATES NewState)
{
	if (BufferPtr->BufferState != NewState)
	{
		PendingResourceBarriers[PendingResourceBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.pResource = BufferPtr->DXBuffer;
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.StateAfter = NewState;
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.StateBefore = BufferPtr->BufferState;
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.Subresource = 0;
		PendingResourceBarriers[PendingResourceBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		++PendingResourceBarriersCount;

		BufferPtr->BufferState = NewState;
	}
}

void RenderSystem::SetTextureState(Pointer<Texture>& TexturePtr, UINT SubResourceIndex, D3D12_RESOURCE_STATES NewState)
{
	if (TexturePtr->TextureSubResourceStates[SubResourceIndex] != NewState)
	{
		PendingResourceBarriers[PendingResourceBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.pResource = TexturePtr->DXTexture;
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.StateAfter = NewState;
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.StateBefore = TexturePtr->TextureSubResourceStates[SubResourceIndex];
		PendingResourceBarriers[PendingResourceBarriersCount].Transition.Subresource = SubResourceIndex;
		PendingResourceBarriers[PendingResourceBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		++PendingResourceBarriersCount;

		TexturePtr->TextureSubResourceStates[SubResourceIndex] = NewState;
	}
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
	ResourceDesc.Width = sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount;

	D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	size_t AlignedResourceOffset = (BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] == 0) ? 0 : BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] + (ResourceAllocationInfo.Alignment - BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] % ResourceAllocationInfo.Alignment);

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

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex])));

		AlignedResourceOffset = 0;
	}

	SAFE_DX(Device->CreatePlacedResource(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(renderMesh->MeshBuffer)));

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes;

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
	memcpy((BYTE*)MappedData, renderMeshCreateInfo.MeshData, sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount);
	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CopyCommandAllocator->Reset());
	SAFE_DX(CopyCommandList->Reset(CopyCommandAllocator, nullptr));

	CopyCommandList->CopyBufferRegion(renderMesh->MeshBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount);

	/*D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = renderMesh->MeshBuffer;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);*/

	SAFE_DX(CopyCommandList->Close());

	CopyCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CopyCommandList);

	SAFE_DX(CopyCommandQueue->Signal(CopySyncFence, 1));

	if (CopySyncFence->GetCompletedValue() != 1)
	{
		SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
		DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
	}

	SAFE_DX(CopySyncFence->Signal(0));

	renderMesh->VertexBufferAddresses[0] = renderMesh->MeshBuffer->GetGPUVirtualAddress();
	renderMesh->VertexBufferAddresses[1] = renderMesh->VertexBufferAddresses[0] + sizeof(XMFLOAT3) * renderMeshCreateInfo.VertexCount;
	renderMesh->VertexBufferAddresses[2] = renderMesh->VertexBufferAddresses[1] + sizeof(XMFLOAT2) * renderMeshCreateInfo.VertexCount;
	renderMesh->IndexBufferAddress = renderMesh->VertexBufferAddresses[2] + 3 * sizeof(XMFLOAT3) * renderMeshCreateInfo.VertexCount;

	return renderMesh;

	return renderMesh;
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTexture *renderTexture = new RenderTexture();

	DXGI_FORMAT TextureFormat;

	if (renderTextureCreateInfo.Compressed)
	{
		if (renderTextureCreateInfo.SRGB)
		{
			switch (renderTextureCreateInfo.CompressionType)
			{
				case BlockCompression::BC1:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB;
					break;
				case BlockCompression::BC2:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB;
					break;
				case BlockCompression::BC3:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB;
					break;
				default:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
					break;
			}
		}
		else
		{
			switch (renderTextureCreateInfo.CompressionType)
			{
				case BlockCompression::BC1:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;
					break;
				case BlockCompression::BC2:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM;
					break;
				case BlockCompression::BC3:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;
					break;
				case BlockCompression::BC4:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM;
					break;
				case BlockCompression::BC5:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM;
					break;
				default:
					TextureFormat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
					break;
			}
		}
	}
	else
	{
		if (renderTextureCreateInfo.SRGB)
		{
			TextureFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		else
		{
			TextureFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = TextureFormat;
	ResourceDesc.Height = renderTextureCreateInfo.Height;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = renderTextureCreateInfo.MIPLevels;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = renderTextureCreateInfo.Width;

	D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	size_t AlignedResourceOffset = (TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] == 0) ? 0 : TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] + (ResourceAllocationInfo.Alignment - TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] % ResourceAllocationInfo.Alignment);

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

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex])));

		AlignedResourceOffset = 0;
	}

	SAFE_DX(Device->CreatePlacedResource(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex], AlignedResourceOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, nullptr, UUIDOF(renderTexture->Texture)));

	TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] = AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrints[MAX_MIP_LEVELS_IN_TEXTURE];

	UINT NumsRows[MAX_MIP_LEVELS_IN_TEXTURE];
	UINT64 RowsSizesInBytes[MAX_MIP_LEVELS_IN_TEXTURE], TotalBytes;

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

		if (renderTextureCreateInfo.Compressed)
		{
			if (renderTextureCreateInfo.CompressionType == BlockCompression::BC1)
				TexelData += 8 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
			else if (renderTextureCreateInfo.CompressionType == BlockCompression::BC5)
				TexelData += 16 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
		}
		else
		{
			TexelData += 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
		}
	}

	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CopyCommandAllocator->Reset());
	SAFE_DX(CopyCommandList->Reset(CopyCommandAllocator, nullptr));

	D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

	SourceTextureCopyLocation.pResource = UploadBuffer;
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	DestTextureCopyLocation.pResource = renderTexture->Texture;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	for (UINT i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
	{
		SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrints[i];
		DestTextureCopyLocation.SubresourceIndex = i;

		CopyCommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);
	}

	/*D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = renderTexture->Texture;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CopyCommandList->ResourceBarrier(1, &ResourceBarrier);*/

	SAFE_DX(CopyCommandList->Close());

	CopyCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CopyCommandList);

	SAFE_DX(CopyCommandQueue->Signal(CopySyncFence, 1));

	if (CopySyncFence->GetCompletedValue() != 1)
	{
		SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
		DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
	}

	SAFE_DX(CopySyncFence->Signal(0));

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = TextureFormat;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = renderTextureCreateInfo.MIPLevels;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	renderTexture->TextureSRV = TexturesDescriptorHeap->AllocateDescriptor();

	Device->CreateShaderResourceView(renderTexture->Texture, &SRVDesc, renderTexture->TextureSRV);

	return renderTexture;
}

RenderMaterial* RenderSystem::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterial *renderMaterial = new RenderMaterial();

	D3D12_INPUT_ELEMENT_DESC InputElementDescs[5];
	InputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 1;
	InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";
	InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[2].InputSlot = 2;
	InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[2].InstanceDataStepRate = 0;
	InputElementDescs[2].SemanticIndex = 0;
	InputElementDescs[2].SemanticName = "NORMAL";
	InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[3].InputSlot = 2;
	InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[3].InstanceDataStepRate = 0;
	InputElementDescs[3].SemanticIndex = 0;
	InputElementDescs[3].SemanticName = "TANGENT";
	InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[4].InputSlot = 2;
	InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[4].InstanceDataStepRate = 0;
	InputElementDescs[4].SemanticIndex = 0;
	InputElementDescs[4].SemanticName = "BINORMAL";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 3;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.RTVFormats[1] = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	GraphicsPipelineStateDesc.RTVFormats[2] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(renderMaterial->GBufferOpaquePassPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ALL;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 1;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeLength;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(renderMaterial->ShadowMapPassPipelineState)));

	return renderMaterial;
}

void RenderSystem::DestroyRenderMesh(RenderMesh* renderMesh)
{
	RenderMeshDestructionQueue.Add(renderMesh);
}

void RenderSystem::DestroyRenderTexture(RenderTexture* renderTexture)
{
	RenderTextureDestructionQueue.Add(renderTexture);
}

void RenderSystem::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	RenderMaterialDestructionQueue.Add(renderMaterial);
}

inline void RenderSystem::CheckDXCallResult(HRESULT hr, const char16_t* Function)
{
	if (FAILED(hr))
	{
		char16_t DXErrorMessageBuffer[2048];
		char16_t DXErrorCodeBuffer[512];

		const char16_t *DXErrorCodePtr = RenderSystem::GetDXErrorMessageFromHRESULT(hr);

		if (DXErrorCodePtr) wcscpy((wchar_t*)DXErrorCodeBuffer, (const wchar_t*)DXErrorCodePtr);
		else wsprintf((wchar_t*)DXErrorCodeBuffer, (const wchar_t*)u"0x%08X (неизвестный код)", hr);

		wsprintf((wchar_t*)DXErrorMessageBuffer, (const wchar_t*)u"Произошла ошибка при попытке вызова следующей DirectX-функции:\r\n%s\r\nКод ошибки: %s", (const wchar_t*)Function, (const wchar_t*)DXErrorCodeBuffer);

		int IntResult = MessageBox(NULL, (const wchar_t*)DXErrorMessageBuffer, (const wchar_t*)u"Ошибка DirectX", MB_OK | MB_ICONERROR);

		if (hr == DXGI_ERROR_DEVICE_REMOVED) SAFE_DX(Device->GetDeviceRemovedReason());

		ExitProcess(0);
	}
}

inline const char16_t* RenderSystem::GetDXErrorMessageFromHRESULT(HRESULT hr)
{
	switch (hr)
	{
		case E_UNEXPECTED:
			return u"E_UNEXPECTED";
			break;
		case E_NOTIMPL:
			return u"E_NOTIMPL";
			break;
		case E_OUTOFMEMORY:
			return u"E_OUTOFMEMORY";
			break;
		case E_INVALIDARG:
			return u"E_INVALIDARG";
			break;
		case E_NOINTERFACE:
			return u"E_NOINTERFACE";
			break;
		case E_POINTER:
			return u"E_POINTER";
			break;
		case E_HANDLE:
			return u"E_HANDLE";
			break;
		case E_ABORT:
			return u"E_ABORT";
			break;
		case E_FAIL:
			return u"E_FAIL";
			break;
		case E_ACCESSDENIED:
			return u"E_ACCESSDENIED";
			break;
		case E_PENDING:
			return u"E_PENDING";
			break;
		case E_BOUNDS:
			return u"E_BOUNDS";
			break;
		case E_CHANGED_STATE:
			return u"E_CHANGED_STATE";
			break;
		case E_ILLEGAL_STATE_CHANGE:
			return u"E_ILLEGAL_STATE_CHANGE";
			break;
		case E_ILLEGAL_METHOD_CALL:
			return u"E_ILLEGAL_METHOD_CALL";
			break;
		case E_STRING_NOT_NULL_TERMINATED:
			return u"E_STRING_NOT_NULL_TERMINATED";
			break;
		case E_ILLEGAL_DELEGATE_ASSIGNMENT:
			return u"E_ILLEGAL_DELEGATE_ASSIGNMENT";
			break;
		case E_ASYNC_OPERATION_NOT_STARTED:
			return u"E_ASYNC_OPERATION_NOT_STARTED";
			break;
		case E_APPLICATION_EXITING:
			return u"E_APPLICATION_EXITING";
			break;
		case E_APPLICATION_VIEW_EXITING:
			return u"E_APPLICATION_VIEW_EXITING";
			break;
		case DXGI_ERROR_INVALID_CALL:
			return u"DXGI_ERROR_INVALID_CALL";
			break;
		case DXGI_ERROR_NOT_FOUND:
			return u"DXGI_ERROR_NOT_FOUND";
			break;
		case DXGI_ERROR_MORE_DATA:
			return u"DXGI_ERROR_MORE_DATA";
			break;
		case DXGI_ERROR_UNSUPPORTED:
			return u"DXGI_ERROR_UNSUPPORTED";
			break;
		case DXGI_ERROR_DEVICE_REMOVED:
			return u"DXGI_ERROR_DEVICE_REMOVED";
			break;
		case DXGI_ERROR_DEVICE_HUNG:
			return u"DXGI_ERROR_DEVICE_HUNG";
			break;
		case DXGI_ERROR_DEVICE_RESET:
			return u"DXGI_ERROR_DEVICE_RESET";
			break;
		case DXGI_ERROR_WAS_STILL_DRAWING:
			return u"DXGI_ERROR_WAS_STILL_DRAWING";
			break;
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
			return u"DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
			break;
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
			return u"DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";
			break;
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			return u"DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break;
		case DXGI_ERROR_NONEXCLUSIVE:
			return u"DXGI_ERROR_NONEXCLUSIVE";
			break;
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			return u"DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
			break;
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
			return u"DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
			break;
		case DXGI_ERROR_REMOTE_OUTOFMEMORY:
			return u"DXGI_ERROR_REMOTE_OUTOFMEMORY";
			break;
		case DXGI_ERROR_ACCESS_LOST:
			return u"DXGI_ERROR_ACCESS_LOST";
			break;
		case DXGI_ERROR_WAIT_TIMEOUT:
			return u"DXGI_ERROR_WAIT_TIMEOUT";
			break;
		case DXGI_ERROR_SESSION_DISCONNECTED:
			return u"DXGI_ERROR_SESSION_DISCONNECTED";
			break;
		case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
			return u"DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";
			break;
		case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
			return u"DXGI_ERROR_CANNOT_PROTECT_CONTENT";
			break;
		case DXGI_ERROR_ACCESS_DENIED:
			return u"DXGI_ERROR_ACCESS_DENIED";
			break;
		case DXGI_ERROR_NAME_ALREADY_EXISTS:
			return u"DXGI_ERROR_NAME_ALREADY_EXISTS";
			break;
		case DXGI_ERROR_SDK_COMPONENT_MISSING:
			return u"DXGI_ERROR_SDK_COMPONENT_MISSING";
			break;
		case DXGI_ERROR_NOT_CURRENT:
			return u"DXGI_ERROR_NOT_CURRENT";
			break;
		case DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY:
			return u"DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY";
			break;
		case DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION:
			return u"DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION";
			break;
		case DXGI_ERROR_NON_COMPOSITED_UI:
			return u"DXGI_ERROR_NON_COMPOSITED_UI";
			break;
		case DXGI_ERROR_MODE_CHANGE_IN_PROGRESS:
			return u"DXGI_ERROR_MODE_CHANGE_IN_PROGRESS";
			break;
		case DXGI_ERROR_CACHE_CORRUPT:
			return u"DXGI_ERROR_CACHE_CORRUPT";
			break;
		case DXGI_ERROR_CACHE_FULL:
			return u"DXGI_ERROR_CACHE_FULL";
			break;
		case DXGI_ERROR_CACHE_HASH_COLLISION:
			return u"DXGI_ERROR_CACHE_HASH_COLLISION";
			break;
		case DXGI_ERROR_ALREADY_EXISTS:
			return u"DXGI_ERROR_ALREADY_EXISTS";
			break;
		case D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return u"D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break;
		case D3D10_ERROR_FILE_NOT_FOUND:
			return u"D3D10_ERROR_FILE_NOT_FOUND";
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return u"D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break;
		case D3D11_ERROR_FILE_NOT_FOUND:
			return u"D3D11_ERROR_FILE_NOT_FOUND";
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
			return u"D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
			break;
		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
			return u"D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
			break;
		case D3D12_ERROR_ADAPTER_NOT_FOUND:
			return u"D3D12_ERROR_ADAPTER_NOT_FOUND";
			break;
		case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
			return u"D3D12_ERROR_DRIVER_VERSION_MISMATCH";
			break;
		default:
			return nullptr;
			break;
	}

	return nullptr;
}