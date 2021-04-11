// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderDeviceD3D12.h"

#include "RenderPasses/GBufferOpaquePass.h"
#include "RenderPasses/MSAADepthBufferResolvePass.h"
#include "RenderPasses/OcclusionBufferPass.h"
#include "RenderPasses/ShadowMapPass.h"
#include "RenderPasses/ShadowResolvePass.h"
#include "RenderPasses/DeferredLightingPass.h"
#include "RenderPasses/SkyAndFogPass.h"
#include "RenderPasses/HDRSceneColorResolvePass.h"
#include "RenderPasses/PostProcessLuminancePass.h"
#include "RenderPasses/PostProcessBloomPass.h"
#include "RenderPasses/PostProcessHDRToneMappingPass.h"
#include "RenderPasses/BackBufferResolvePass.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

DescriptorHeap::DescriptorHeap(ID3D12Device *DXDevice, const D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType, const UINT DescriptorsCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = DescriptorsCount;
	DescriptorHeapDesc.Type = DescriptorHeapType;

	// TODO: Сделать обработку ошибок
	DXDevice->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(DXDescriptorHeap));

	FirstDescriptor = DXDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
	DescriptorSize = DXDevice->GetDescriptorHandleIncrementSize(DescriptorHeapType);

	AllocatedDescriptorsCount = 0;
}

RootSignature::RootSignature(ID3D12Device *DXDevice, const D3D12_ROOT_SIGNATURE_DESC& RootSignatureDesc)
{
	COMRCPtr<ID3DBlob> RootSignatureBlob;

	// TODO: Сделать обработку ошибок
	D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0, &RootSignatureBlob, nullptr);
	// TODO: Сделать обработку ошибок
	DXDevice->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), UUIDOF(DXRootSignature));

	DXRootSignatureDesc = RootSignatureDesc;

	DXRootSignatureDesc.pParameters = new D3D12_ROOT_PARAMETER[DXRootSignatureDesc.NumParameters];
	DXRootSignatureDesc.pStaticSamplers = new D3D12_STATIC_SAMPLER_DESC[DXRootSignatureDesc.NumStaticSamplers];

	for (size_t i = 0; i < DXRootSignatureDesc.NumParameters; i++)
	{
		((D3D12_ROOT_PARAMETER*)DXRootSignatureDesc.pParameters)[i] = RootSignatureDesc.pParameters[i];

		if (DXRootSignatureDesc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			((D3D12_ROOT_PARAMETER*)DXRootSignatureDesc.pParameters)[i].DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[DXRootSignatureDesc.pParameters[i].DescriptorTable.NumDescriptorRanges];

			for (size_t j = 0; j < DXRootSignatureDesc.pParameters[i].DescriptorTable.NumDescriptorRanges; j++)
			{
				((D3D12_DESCRIPTOR_RANGE*)((D3D12_ROOT_PARAMETER*)DXRootSignatureDesc.pParameters)[i].DescriptorTable.pDescriptorRanges)[j] = RootSignatureDesc.pParameters[i].DescriptorTable.pDescriptorRanges[j];
			}
		}
	}

	for (size_t i = 0; i < DXRootSignatureDesc.NumStaticSamplers; i++)
	{
		((D3D12_STATIC_SAMPLER_DESC*)DXRootSignatureDesc.pStaticSamplers)[i] = RootSignatureDesc.pStaticSamplers[i];
	}
}

RootSignature::~RootSignature()
{
	for (size_t i = 0; i < DXRootSignatureDesc.NumParameters; i++)
	{
		if (DXRootSignatureDesc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			delete[] DXRootSignatureDesc.pParameters[i].DescriptorTable.pDescriptorRanges;
		}
	}

	delete[] DXRootSignatureDesc.pParameters;
	delete[] DXRootSignatureDesc.pStaticSamplers;
}

FrameDescriptorHeap::FrameDescriptorHeap(ID3D12Device *DXDevice, const D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType, const UINT DescriptorsCount, UINT* FrameIndexRef) : DescriptorHeapType(DescriptorHeapType), CurrentFrameIndex(FrameIndexRef)
{
	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = DescriptorsCount;
	DescriptorHeapDesc.Type = DescriptorHeapType;

	// TODO: Сделать обработку ошибок
	DXDevice->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(DXDescriptorHeaps[0]));
	DXDevice->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(DXDescriptorHeaps[1]));

	FirstDescriptorsCPU[0] = DXDescriptorHeaps[0]->GetCPUDescriptorHandleForHeapStart().ptr;
	FirstDescriptorsCPU[1] = DXDescriptorHeaps[1]->GetCPUDescriptorHandleForHeapStart().ptr;
	FirstDescriptorsGPU[0] = DXDescriptorHeaps[0]->GetGPUDescriptorHandleForHeapStart().ptr;
	FirstDescriptorsGPU[1] = DXDescriptorHeaps[1]->GetGPUDescriptorHandleForHeapStart().ptr;
	DescriptorSize = DXDevice->GetDescriptorHandleIncrementSize(DescriptorHeapType);

	Device = DXDevice;
}

DescriptorTable FrameDescriptorHeap::AllocateDescriptorTable(const D3D12_ROOT_PARAMETER& RootParameter)
{
	UINT DescriptorsCountInTable = 0;

	for (UINT i = 0; i < RootParameter.DescriptorTable.NumDescriptorRanges; i++)
	{
		DescriptorsCountInTable += RootParameter.DescriptorTable.pDescriptorRanges[i].NumDescriptors;
	}

	DescriptorTable descriptorTable(
		DescriptorsCountInTable,
		AllocatedDescriptorsForTables,
		FirstDescriptorsCPU[0] + AllocatedDescriptorsForTables * DescriptorSize,
		FirstDescriptorsCPU[1] + AllocatedDescriptorsForTables * DescriptorSize,
		FirstDescriptorsGPU[0] + AllocatedDescriptorsForTables * DescriptorSize,
		FirstDescriptorsGPU[1] + AllocatedDescriptorsForTables * DescriptorSize,
		DescriptorHeapType,
		Device,
		CurrentFrameIndex
	);

	AllocatedDescriptorsForTables += DescriptorsCountInTable;

	return descriptorTable;
}

void RenderDeviceD3D12::InitDevice()
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

	ResolutionWidth = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Screen", "ResolutionWidth");
	ResolutionHeight = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Screen", "ResolutionHeight");

	int HardwareAntiAliasingMode = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Graphics", "HardwareAntiAliasingMode");
	int HardwareAntiAliasingLevel = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Graphics", "HardwareAntiAliasingLevel");

	int SamplesCount = (HardwareAntiAliasingMode > 0) ? 1 << (HardwareAntiAliasingLevel + 1) : 1;

	SAFE_DX(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0, UUIDOF(Device)));

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc;
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.NodeMask = 0;
	CommandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT;

	SAFE_DX(Device->CreateCommandQueue(&CommandQueueDesc, UUIDOF(CommandQueue)));

	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, UUIDOF(CommandAllocators[0])));
	SAFE_DX(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, UUIDOF(CommandAllocators[1])));

	SAFE_DX(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0], nullptr, UUIDOF(CommandList)));
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
	SAFE_DX(Factory->CreateSwapChainForHwnd(CommandQueue, RenderTargetHandle, &SwapChainDesc, &SwapChainFullScreenDesc, nullptr, &SwapChain1));
	SAFE_DX(SwapChain1->QueryInterface<IDXGISwapChain4>(&SwapChain));

#if WITH_EDITOR
	if (!Application::IsEditor())
#endif
		SAFE_DX(Factory->MakeWindowAssociation(RenderTargetHandle, DXGI_MWA_NO_ALT_ENTER));

	delete[] DisplayModes;

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CurrentFrameIndex = 0;

	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(FrameSyncFences[0])));
	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(FrameSyncFences[1])));

	FrameSyncEvent = CreateEvent(NULL, FALSE, FALSE, (const wchar_t*)u"FrameSyncEvent");

	SAFE_DX(Device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(CopySyncFence)));

	CopySyncEvent = CreateEvent(NULL, FALSE, FALSE, (const wchar_t*)u"CopySyncEvent");

	new (&RTDescriptorHeap) DescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 29);
	new (&DSDescriptorHeap) DescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5);
	new (&CBSRUADescriptorHeap) DescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 49);
	new (&SamplersDescriptorHeap) DescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 4);
	new (&ConstantBufferDescriptorHeap) DescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100000);
	new (&TexturesDescriptorHeap) DescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8002);

	new (&FrameResourcesDescriptorHeap) FrameDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 500000, &CurrentFrameIndex);
	new (&FrameSamplersDescriptorHeap) FrameDescriptorHeap(Device, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2000, &CurrentFrameIndex);

	D3D12_DESCRIPTOR_RANGE DescriptorRanges[4];
	DescriptorRanges[0].BaseShaderRegister = 0;
	DescriptorRanges[0].NumDescriptors = 1;
	DescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;
	DescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescriptorRanges[0].RegisterSpace = 0;
	DescriptorRanges[1].BaseShaderRegister = 0;
	DescriptorRanges[1].NumDescriptors = 7;
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

	D3D12_ROOT_PARAMETER GraphicsRootParameters[6];
	GraphicsRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	GraphicsRootParameters[0].DescriptorTable.pDescriptorRanges = &DescriptorRanges[0];
	GraphicsRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	GraphicsRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
	GraphicsRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	GraphicsRootParameters[1].DescriptorTable.pDescriptorRanges = &DescriptorRanges[1];
	GraphicsRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	GraphicsRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
	GraphicsRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	GraphicsRootParameters[2].DescriptorTable.pDescriptorRanges = &DescriptorRanges[2];
	GraphicsRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	GraphicsRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX;
	GraphicsRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	GraphicsRootParameters[3].DescriptorTable.pDescriptorRanges = &DescriptorRanges[0];
	GraphicsRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	GraphicsRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
	GraphicsRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	GraphicsRootParameters[4].DescriptorTable.pDescriptorRanges = &DescriptorRanges[1];
	GraphicsRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	GraphicsRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;
	GraphicsRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	GraphicsRootParameters[5].DescriptorTable.pDescriptorRanges = &DescriptorRanges[2];
	GraphicsRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	GraphicsRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC GraphicsRootSignatureDesc;
	GraphicsRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	GraphicsRootSignatureDesc.NumParameters = 6;
	GraphicsRootSignatureDesc.NumStaticSamplers = 0;
	GraphicsRootSignatureDesc.pParameters = GraphicsRootParameters;
	GraphicsRootSignatureDesc.pStaticSamplers = nullptr;

	new (&GraphicsRootSignature) RootSignature(Device, GraphicsRootSignatureDesc);

	D3D12_ROOT_PARAMETER ComputeRootParameters[6];
	ComputeRootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	ComputeRootParameters[0].DescriptorTable.pDescriptorRanges = &DescriptorRanges[0];
	ComputeRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	ComputeRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	ComputeRootParameters[1].DescriptorTable.pDescriptorRanges = &DescriptorRanges[1];
	ComputeRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	ComputeRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	ComputeRootParameters[2].DescriptorTable.pDescriptorRanges = &DescriptorRanges[2];
	ComputeRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
	ComputeRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	ComputeRootParameters[3].DescriptorTable.pDescriptorRanges = &DescriptorRanges[3];
	ComputeRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ComputeRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC ComputeRootSignatureDesc;
	ComputeRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	ComputeRootSignatureDesc.NumParameters = 4;
	ComputeRootSignatureDesc.NumStaticSamplers = 0;
	ComputeRootSignatureDesc.pParameters = ComputeRootParameters;
	ComputeRootSignatureDesc.pStaticSamplers = nullptr;

	new (&ComputeRootSignature) RootSignature(Device, ComputeRootSignatureDesc);

	{
		SAFE_DX(SwapChain->GetBuffer(0, UUIDOF(BackBufferTextures[0].DXTexture)));
		SAFE_DX(SwapChain->GetBuffer(1, UUIDOF(BackBufferTextures[1].DXTexture)));

		BackBufferTextures[0].DXTextureSubResourceStates = new D3D12_RESOURCE_STATES;
		BackBufferTextures[0].DXTextureSubResourceStates[0] = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
		BackBufferTextures[1].DXTextureSubResourceStates = new D3D12_RESOURCE_STATES;
		BackBufferTextures[1].DXTextureSubResourceStates[0] = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		BackBufferTexturesRTVs[0] = RTDescriptorHeap.AllocateDescriptor();
		BackBufferTexturesRTVs[1] = RTDescriptorHeap.AllocateDescriptor();

		Device->CreateRenderTargetView(BackBufferTextures[0].DXTexture, &RTVDesc, BackBufferTexturesRTVs[0]);
		Device->CreateRenderTargetView(BackBufferTextures[1].DXTexture, &RTVDesc, BackBufferTexturesRTVs[1]);
	}

	// ===============================================================================================================

	FullScreenQuadVertexShader.BytecodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("ShaderModel51.FullScreenQuad");
	ScopedMemoryBlockArray<BYTE> FullScreenQuadVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FullScreenQuadVertexShader.BytecodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("ShaderModel51.FullScreenQuad", FullScreenQuadVertexShaderByteCodeData);
	FullScreenQuadVertexShader.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

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

		TextureSampler = SamplersDescriptorHeap.AllocateDescriptor();

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

		ShadowMapSampler = SamplersDescriptorHeap.AllocateDescriptor();

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

		BiLinearSampler = SamplersDescriptorHeap.AllocateDescriptor();

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

		MinSampler = SamplersDescriptorHeap.AllocateDescriptor();

		Device->CreateSampler(&SamplerDesc, MinSampler);

		TextureSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(GraphicsRootSignature.GetRootSignatureDesc().pParameters[PIXEL_SHADER_SAMPLERS]);
		ShadowMapSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(GraphicsRootSignature.GetRootSignatureDesc().pParameters[PIXEL_SHADER_SAMPLERS]);
		BiLinearSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(GraphicsRootSignature.GetRootSignatureDesc().pParameters[PIXEL_SHADER_SAMPLERS]);
		MinSamplerTable = FrameSamplersDescriptorHeap.AllocateDescriptorTable(GraphicsRootSignature.GetRootSignatureDesc().pParameters[PIXEL_SHADER_SAMPLERS]);
	}

	{
		D3D12_HEAP_DESC HeapDesc;
		ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		HeapDesc.Properties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);
		HeapDesc.SizeInBytes = BUFFER_MEMORY_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex])));

		ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		HeapDesc.Properties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);
		HeapDesc.SizeInBytes = TEXTURE_MEMORY_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex])));

		ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
		HeapDesc.Properties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
		HeapDesc.SizeInBytes = UPLOAD_HEAP_SIZE;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(UploadHeap)));

		D3D12_RESOURCE_DESC ResourceDesc = DX12Helpers::CreateDXResourceDescBuffer(UPLOAD_HEAP_SIZE);

		SAFE_DX(Device->CreatePlacedResource(UploadHeap, 0, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(UploadBuffer)));
	}

	RenderPasses.Add(new GBufferOpaquePass);
	RenderPasses.Add(new MSAADepthBufferResolvePass);
	RenderPasses.Add(new OcclusionBufferPass);
	RenderPasses.Add(new ShadowMapPass);
	RenderPasses.Add(new ShadowResolvePass);
	RenderPasses.Add(new DeferredLightingPass);
	RenderPasses.Add(new SkyAndFogPass);
	RenderPasses.Add(new HDRSceneColorResolvePass);
	RenderPasses.Add(new PostProcessLuminancePass);
	RenderPasses.Add(new PostProcessBloomPass);
	RenderPasses.Add(new PostProcessHDRToneMappingPass);
	RenderPasses.Add(new BackBufferResolvePass);

	//for (RenderPass* renderPass : RenderPasses)
	for (size_t i = 0; i < RenderPasses.GetLength(); i++)
	{
		RenderPass* renderPass = RenderPasses[i];
		renderPass->Init(*this);
	}
}

void RenderDeviceD3D12::ShutdownDevice()
{
	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;

	if (FrameSyncFences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		SAFE_DX(FrameSyncFences[CurrentFrameIndex]->SetEventOnCompletion(1, FrameSyncEvent));
		DWORD WaitResult = WaitForSingleObject(FrameSyncEvent, INFINITE);
	}

	//for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	for (size_t i = 0; i < RenderMeshDestructionQueue.GetLength(); i++)
	{
		RenderMesh* renderMesh = RenderMeshDestructionQueue[i];
		delete (RenderMeshD3D12*)renderMesh;
	}

	//RenderMeshDestructionQueue.clear();

	//for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	for (size_t i = 0; i < RenderMeshDestructionQueue.GetLength(); i++) 
	{
		RenderMaterial* renderMaterial = RenderMaterialDestructionQueue[i];
		delete (RenderMaterialD3D12*)renderMaterial;
	}

	//RenderMaterialDestructionQueue.clear();

	//for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	for (size_t i = 0; i < RenderMeshDestructionQueue.GetLength(); i++)
	{
		RenderTexture* renderTexture = RenderTextureDestructionQueue[i];
		delete (RenderTextureD3D12*)renderTexture;
	}

	//RenderTextureDestructionQueue.clear();

	BOOL Result;

	Result = CloseHandle(FrameSyncEvent);

	//for (RenderPass* renderPass : RenderPasses)
	for (size_t i = 0; i < RenderPasses.GetLength(); i++)
	{
		RenderPass* renderPass = RenderPasses[i];
		delete renderPass;
	}
}

void RenderDeviceD3D12::TickDevice(float DeltaTime)
{
#if WITH_EDITOR
	if (Application::IsEditor())
	{
		Engine::GetEngine().GetGameFramework().GetCamera().SetAspectRatio((float)EditorViewportWidth / (float)EditorViewportHeight);
	}
#endif

	int HardwareAntiAliasingMode = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Graphics", "HardwareAntiAliasingMode");
	int HardwareAntiAliasingLevel = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Graphics", "HardwareAntiAliasingLevel");

	int SamplesCount = (HardwareAntiAliasingMode > 0) ? 1 << (HardwareAntiAliasingLevel + 1) : 1;

	if (FrameSyncFences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		SAFE_DX(FrameSyncFences[CurrentFrameIndex]->SetEventOnCompletion(1, FrameSyncEvent));
		DWORD WaitResult = WaitForSingleObject(FrameSyncEvent, INFINITE);
	}

	SAFE_DX(FrameSyncFences[CurrentFrameIndex]->Signal(0));

	SAFE_DX(CommandAllocators[CurrentFrameIndex]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[CurrentFrameIndex], nullptr));

	ID3D12DescriptorHeap *DescriptorHeaps[2] = { FrameResourcesDescriptorHeap.GetDXDescriptorHeap(), FrameSamplersDescriptorHeap.GetDXDescriptorHeap() };

	CommandList->SetDescriptorHeaps(2, DescriptorHeaps);
	CommandList->SetGraphicsRootSignature(GraphicsRootSignature);
	CommandList->SetComputeRootSignature(ComputeRootSignature);

	TextureSamplerTable[0] = TextureSampler;
	TextureSamplerTable.SetTableSize(1);
	TextureSamplerTable.UpdateDescriptorTable();

	ShadowMapSamplerTable[0] = ShadowMapSampler;
	ShadowMapSamplerTable.SetTableSize(1);
	ShadowMapSamplerTable.UpdateDescriptorTable();

	BiLinearSamplerTable[0] = BiLinearSampler;
	BiLinearSamplerTable.SetTableSize(1);
	BiLinearSamplerTable.UpdateDescriptorTable();

	MinSamplerTable[0] = MinSampler;
	MinSamplerTable.SetTableSize(1);
	MinSamplerTable.UpdateDescriptorTable();

	//for (RenderPass* renderPass : RenderPasses)
	for (size_t i = 0; i < RenderPasses.GetLength(); i++)
	{
		RenderPass* renderPass = RenderPasses[i];
		renderPass->Execute(*this);
	}

	SwitchResourceState(BackBufferTextures[CurrentFrameIndex], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT);
	ApplyPendingBarriers();

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));

	SAFE_DX(CommandQueue->Signal(FrameSyncFences[CurrentFrameIndex], 1));

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

/*void RenderSystem::SwitchResourceState(ID3D12Resource *Resource, const UINT SubResource, const D3D12_RESOURCE_STATES OldState, const D3D12_RESOURCE_STATES NewState)
{
	PendingResourceBarriers[PendingBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	PendingResourceBarriers[PendingBarriersCount].Transition.pResource = Resource;
	PendingResourceBarriers[PendingBarriersCount].Transition.StateAfter = NewState;
	PendingResourceBarriers[PendingBarriersCount].Transition.StateBefore = OldState;
	PendingResourceBarriers[PendingBarriersCount].Transition.Subresource = SubResource;
	PendingResourceBarriers[PendingBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	++PendingBarriersCount;
}*/

void RenderDeviceD3D12::SwitchResourceState(Buffer& buffer, const D3D12_RESOURCE_STATES NewState)
{
	if (buffer.DXBufferState != NewState)
	{
		PendingResourceBarriers[PendingBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		PendingResourceBarriers[PendingBarriersCount].Transition.pResource = buffer.DXBuffer;
		PendingResourceBarriers[PendingBarriersCount].Transition.StateAfter = NewState;
		PendingResourceBarriers[PendingBarriersCount].Transition.StateBefore = buffer.DXBufferState;
		PendingResourceBarriers[PendingBarriersCount].Transition.Subresource = 0;
		PendingResourceBarriers[PendingBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		++PendingBarriersCount;

		buffer.DXBufferState = NewState;
	}
}

void RenderDeviceD3D12::SwitchResourceState(Texture& texture, const UINT SubResource, const D3D12_RESOURCE_STATES NewState)
{
	if (SubResource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		bool AreAllSubResourcesInSameState = true;

		for (UINT i = 1; i < texture.SubResourcesCount; i++)
		{
			if (texture.DXTextureSubResourceStates[i] != texture.DXTextureSubResourceStates[0])
			{
				AreAllSubResourcesInSameState = false;
				break;
			}
		}

		if (AreAllSubResourcesInSameState)
		{
			PendingResourceBarriers[PendingBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			PendingResourceBarriers[PendingBarriersCount].Transition.pResource = texture.DXTexture;
			PendingResourceBarriers[PendingBarriersCount].Transition.StateAfter = NewState;
			PendingResourceBarriers[PendingBarriersCount].Transition.StateBefore = texture.DXTextureSubResourceStates[0];
			PendingResourceBarriers[PendingBarriersCount].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			PendingResourceBarriers[PendingBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			++PendingBarriersCount;
		}
		else
		{
			for (UINT i = 0; i < texture.SubResourcesCount; i++)
			{
				if (texture.DXTextureSubResourceStates[i] != NewState)
				{
					PendingResourceBarriers[PendingBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
					PendingResourceBarriers[PendingBarriersCount].Transition.pResource = texture.DXTexture;
					PendingResourceBarriers[PendingBarriersCount].Transition.StateAfter = NewState;
					PendingResourceBarriers[PendingBarriersCount].Transition.StateBefore = texture.DXTextureSubResourceStates[i];
					PendingResourceBarriers[PendingBarriersCount].Transition.Subresource = i;
					PendingResourceBarriers[PendingBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

					++PendingBarriersCount;
				}
			}
		}

		for (UINT i = 0; i < texture.SubResourcesCount; i++)
		{
			texture.DXTextureSubResourceStates[i] = NewState;
		}
	}
	else
	{
		if (texture.DXTextureSubResourceStates[SubResource] != NewState)
		{
			PendingResourceBarriers[PendingBarriersCount].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			PendingResourceBarriers[PendingBarriersCount].Transition.pResource = texture.DXTexture;
			PendingResourceBarriers[PendingBarriersCount].Transition.StateAfter = NewState;
			PendingResourceBarriers[PendingBarriersCount].Transition.StateBefore = texture.DXTextureSubResourceStates[SubResource];
			PendingResourceBarriers[PendingBarriersCount].Transition.Subresource = SubResource;
			PendingResourceBarriers[PendingBarriersCount].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			++PendingBarriersCount;

			texture.DXTextureSubResourceStates[SubResource] = NewState;
		}
	}
}

void RenderDeviceD3D12::ApplyPendingBarriers()
{
	if (PendingBarriersCount > 0)
	{
		CommandList->ResourceBarrier(PendingBarriersCount, PendingResourceBarriers);

		PendingBarriersCount = 0;
	}
}

RenderPass* RenderDeviceD3D12::GetRenderPass(const String& RenderPassName)
{
	//for (RenderPass* renderPass : RenderPasses)
	for (size_t i = 0; i < RenderPasses.GetLength(); i++)
	{
		RenderPass* renderPass = RenderPasses[i];
		if (renderPass->GetName() == RenderPassName)
		{
			return renderPass;
		}
	}

	return nullptr;
}

RenderMesh* RenderDeviceD3D12::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMeshD3D12 *renderMesh = new RenderMeshD3D12();

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

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex])));

		AlignedResourceOffset = 0;
	}

	SAFE_DX(Device->CreatePlacedResource(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(renderMesh->MeshBuffer)));

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + ResourceAllocationInfo.SizeInBytes;

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;

	ReadRange.Begin = ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
	memcpy((BYTE*)MappedData, renderMeshCreateInfo.MeshData, sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount);
	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CommandAllocators[0]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

	CommandList->CopyBufferRegion(renderMesh->MeshBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount);

	D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = renderMesh->MeshBuffer;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = 0;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

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
}

RenderTexture* RenderDeviceD3D12::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTextureD3D12 *renderTexture = new RenderTextureD3D12();

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

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex])));

		AlignedResourceOffset = 0;
	}

	SAFE_DX(Device->CreatePlacedResource(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex], AlignedResourceOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(renderTexture->Texture)));

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

	SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

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

	/*renderTexture->TextureSRV.ptr = TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + TexturesDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	++TexturesDescriptorsCount;*/

	renderTexture->TextureSRV = TexturesDescriptorHeap.AllocateDescriptor();

	Device->CreateShaderResourceView(renderTexture->Texture, &SRVDesc, renderTexture->TextureSRV);

	return renderTexture;
}

RenderMaterial* RenderDeviceD3D12::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterialD3D12 *renderMaterial = new RenderMaterialD3D12();

	int HardwareAntiAliasingMode = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Graphics", "HardwareAntiAliasingMode");
	int HardwareAntiAliasingLevel = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueInt("Graphics", "HardwareAntiAliasingLevel");

	int SamplesCount = (HardwareAntiAliasingMode > 0) ? 1 << (HardwareAntiAliasingLevel + 1) : 1;

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
	GraphicsPipelineStateDesc.NumRenderTargets = 2;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.RTVFormats[1] = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	GraphicsPipelineStateDesc.SampleDesc.Count = SamplesCount;
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

void RenderDeviceD3D12::DestroyRenderMesh(RenderMesh* renderMesh)
{
	RenderMeshDestructionQueue.Add(renderMesh);
}

void RenderDeviceD3D12::DestroyRenderTexture(RenderTexture* renderTexture)
{
	RenderTextureDestructionQueue.Add(renderTexture);
}

void RenderDeviceD3D12::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	RenderMaterialDestructionQueue.Add(renderMaterial);
}

Buffer RenderDeviceD3D12::CreateBuffer(const D3D12_HEAP_PROPERTIES& HeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue)
{
	Buffer buffer;
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, HeapFlags, &ResourceDesc, InitialState, ClearValue, UUIDOF(buffer.DXBuffer)));
	buffer.DXBufferState = InitialState;
	return buffer;
}

Buffer RenderDeviceD3D12::CreateBuffer(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue)
{
	Buffer buffer;
	SAFE_DX(Device->CreatePlacedResource(Heap, HeapOffset, &ResourceDesc, InitialState, ClearValue, UUIDOF(buffer.DXBuffer)));
	buffer.DXBufferState = InitialState;
	return buffer;
}

Texture RenderDeviceD3D12::CreateTexture(const D3D12_HEAP_PROPERTIES& HeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue)
{
	Texture texture;
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, HeapFlags, &ResourceDesc, InitialState, ClearValue, UUIDOF(texture.DXTexture)));
	UINT SubResourcesCount = 0;
	if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		SubResourcesCount = ResourceDesc.DepthOrArraySize * ResourceDesc.MipLevels;

		if (ResourceDesc.Format == DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT || ResourceDesc.Format == DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT)
			SubResourcesCount *= 2;
	}
	else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		SubResourcesCount = ResourceDesc.MipLevels;
	}
	texture.DXTextureSubResourceStates = new D3D12_RESOURCE_STATES[SubResourcesCount];
	for (UINT i = 0; i < SubResourcesCount; i++)
	{
		texture.DXTextureSubResourceStates[i] = InitialState;
	}
	texture.SubResourcesCount = SubResourcesCount;
	return texture;
}

Texture RenderDeviceD3D12::CreateTexture(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue)
{
	Texture texture;
	SAFE_DX(Device->CreatePlacedResource(Heap, HeapOffset, &ResourceDesc, InitialState, ClearValue, UUIDOF(texture.DXTexture)));
	UINT SubResourcesCount = 0;
	if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		SubResourcesCount = ResourceDesc.DepthOrArraySize * ResourceDesc.MipLevels;

		if (ResourceDesc.Format == DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT || ResourceDesc.Format == DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT)
			SubResourcesCount *= 2;
	}
	else if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		SubResourcesCount = ResourceDesc.MipLevels;
	}
	texture.DXTextureSubResourceStates = new D3D12_RESOURCE_STATES[SubResourcesCount];
	for (UINT i = 0; i < SubResourcesCount; i++)
	{
		texture.DXTextureSubResourceStates[i] = InitialState;
	}
	texture.SubResourcesCount = SubResourcesCount;
	return texture;
}

inline void RenderDeviceD3D12::CheckDXCallResult(HRESULT hr, const char16_t* Function)
{
	if (FAILED(hr))
	{
		char16_t DXErrorMessageBuffer[2048];
		char16_t DXErrorCodeBuffer[512];

		const char16_t *DXErrorCodePtr = GetDXErrorMessageFromHRESULT(hr);

		if (DXErrorCodePtr) wcscpy((wchar_t*)DXErrorCodeBuffer, (const wchar_t*)DXErrorCodePtr);
		else wsprintf((wchar_t*)DXErrorCodeBuffer, (const wchar_t*)u"0x%08X (неизвестный код)", hr);

		wsprintf((wchar_t*)DXErrorMessageBuffer, (const wchar_t*)u"Произошла ошибка при попытке вызова следующей DirectX-функции:\r\n%s\r\nКод ошибки: %s", (const wchar_t*)Function, (const wchar_t*)DXErrorCodeBuffer);

		int IntResult = MessageBox(NULL, (const wchar_t*)DXErrorMessageBuffer, (const wchar_t*)u"Ошибка DirectX", MB_OK | MB_ICONERROR);

		if (hr == DXGI_ERROR_DEVICE_REMOVED) SAFE_DX(Device->GetDeviceRemovedReason());

		ExitProcess(0);
	}
}

inline const char16_t* RenderDeviceD3D12::GetDXErrorMessageFromHRESULT(HRESULT hr)
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