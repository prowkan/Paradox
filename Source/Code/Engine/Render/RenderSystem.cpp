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

#include "RenderStages/GBufferOpaqueStage.h"
#include "RenderStages/MSAADepthBufferResolveStage.h"
#include "RenderStages/OcclusionBufferStage.h"
#include "RenderStages/ShadowMapStage.h"
#include "RenderStages/ShadowResolveStage.h"
#include "RenderStages/DeferredLightingStage.h"
#include "RenderStages/SkyAndFogStage.h"
#include "RenderStages/HDRSceneColorResolveStage.h"
#include "RenderStages/PostProcessLuminanceStage.h"
#include "RenderStages/PostProcessBloomStage.h"
#include "RenderStages/PostProcessHDRToneMappingStage.h"
#include "RenderStages/BackBufferResolveStage.h"

struct PointLight
{
	XMFLOAT3 Position;
	float Radius;
	XMFLOAT3 Color;
	float Brightness;
};

struct GBufferOpaquePassConstantBuffer
{
	XMMATRIX WVPMatrix;
	XMMATRIX WorldMatrix;
	XMFLOAT3X4 VectorTransformMatrix;
};

struct ShadowMapPassConstantBuffer
{
	XMMATRIX WVPMatrix;
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

	ResolutionWidth = 1280;
	ResolutionHeight = 720;

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

	COMRCPtr<IDXGISwapChain1> SwapChain1;
	SAFE_DX(Factory->CreateSwapChainForHwnd(CommandQueue, Application::GetMainWindowHandle(), &SwapChainDesc, &SwapChainFullScreenDesc, nullptr, &SwapChain1));
	SAFE_DX(SwapChain1->QueryInterface<IDXGISwapChain4>(&SwapChain));

	SAFE_DX(Factory->MakeWindowAssociation(Application::GetMainWindowHandle(), DXGI_MWA_NO_ALT_ENTER));

	delete[] DisplayModes;

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	CurrentFrameIndex = 0;

	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(FrameSyncFences[0])));
	SAFE_DX(Device->CreateFence(1, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(FrameSyncFences[1])));

	FrameSyncEvent = CreateEvent(NULL, FALSE, FALSE, (const wchar_t*)u"FrameSyncEvent");

	SAFE_DX(Device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, UUIDOF(CopySyncFence)));

	CopySyncEvent = CreateEvent(NULL, FALSE, FALSE, (const wchar_t*)u"CopySyncEvent");

	D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 29;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(RTDescriptorHeap)));

	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 5;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(DSDescriptorHeap)));

	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 49;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(CBSRUADescriptorHeap)));

	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 4;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(SamplersDescriptorHeap)));

	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 100000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(ConstantBufferDescriptorHeap)));

	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 8002;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(TexturesDescriptorHeap)));

	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 500000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(CompiledResourcesDescriptorHeap)));

	ZeroMemory(&DescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescriptorHeapDesc.NodeMask = 0;
	DescriptorHeapDesc.NumDescriptors = 2000;
	DescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	SAFE_DX(Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(CompiledSamplersDescriptorHeap)));

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

	{
		SAFE_DX(SwapChain->GetBuffer(0, UUIDOF(BackBufferTextures[0])));
		SAFE_DX(SwapChain->GetBuffer(1, UUIDOF(BackBufferTextures[1])));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		BackBufferTexturesRTVs[0].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;
		BackBufferTexturesRTVs[1].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;

		Device->CreateRenderTargetView(BackBufferTextures[0], &RTVDesc, BackBufferTexturesRTVs[0]);
		Device->CreateRenderTargetView(BackBufferTextures[1], &RTVDesc, BackBufferTexturesRTVs[1]);
	}

	// ===============================================================================================================

	void *FullScreenQuadVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.FullScreenQuad");
	size_t FullScreenQuadVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.FullScreenQuad");

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

		TextureSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		SamplersDescriptorsCount++;

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

		ShadowMapSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		SamplersDescriptorsCount++;

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

		BiLinearSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		SamplersDescriptorsCount++;

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

		MinSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		SamplersDescriptorsCount++;

		Device->CreateSampler(&SamplerDesc, MinSampler);

		Device->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 0 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) }, TextureSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		Device->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 1 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) }, ShadowMapSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		Device->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 2 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) }, BiLinearSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		Device->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 3 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) }, MinSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	
		TextureSamplerTable = D3D12_GPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + 0 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
		ShadowMapSamplerTable = D3D12_GPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + 1 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
		BiLinearSamplerTable = D3D12_GPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + 2 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
		MinSamplerTable = D3D12_GPU_DESCRIPTOR_HANDLE{ CompiledSamplersDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + 3 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) };
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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &GBufferTexture0ResourceDesc);

		SIZE_T GBufferTexture0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GBufferTexture0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &GBufferTexture1ResourceDesc);

		SIZE_T GBufferTexture1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GBufferTexture1Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &DepthBufferTextureResourceDesc);

		SIZE_T DepthBufferTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = DepthBufferTextureOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &GBufferOpaquePassConstantBufferResourceDesc);

		SIZE_T GPUConstantBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPUConstantBufferOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory1)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory1, GBufferTexture0Offset, &GBufferTexture0ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(GBufferTextures[0])));

		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory1, GBufferTexture1Offset, &GBufferTexture1ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(GBufferTextures[1])));

		ClearValue.DepthStencil.Depth = 0.0f;
		ClearValue.DepthStencil.Stencil = 0;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory1, DepthBufferTextureOffset, &DepthBufferTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue, UUIDOF(DepthBufferTexture)));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory1, GPUConstantBufferOffset, &GBufferOpaquePassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffer)));

		HeapDesc.Alignment = 0;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		SIZE_T CPUConstantBuffer0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUConstantBuffer0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &GBufferOpaquePassConstantBufferResourceDesc);

		SIZE_T CPUConstantBuffer1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUConstantBuffer1Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &GBufferOpaquePassConstantBufferResourceDesc);

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory1)));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory1, CPUConstantBuffer0Offset, &GBufferOpaquePassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory1, CPUConstantBuffer1Offset, &GBufferOpaquePassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[1])));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

		GBufferTexturesRTVs[0].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;
		GBufferTexturesRTVs[1].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		Device->CreateRenderTargetView(GBufferTextures[0], &RTVDesc, GBufferTexturesRTVs[0]);

		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		Device->CreateRenderTargetView(GBufferTextures[1], &RTVDesc, GBufferTexturesRTVs[1]);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

		GBufferTexturesSRVs[0].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;
		GBufferTexturesSRVs[1].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		Device->CreateShaderResourceView(GBufferTextures[0], &SRVDesc, GBufferTexturesSRVs[0]);

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		Device->CreateShaderResourceView(GBufferTextures[1], &SRVDesc, GBufferTexturesSRVs[1]);

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
		DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DMS;

		DepthBufferTextureDSV.ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DSDescriptorsCount++;

		Device->CreateDepthStencilView(DepthBufferTexture, &DSVDesc, DepthBufferTextureDSV);

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

		DepthBufferTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(DepthBufferTexture, &SRVDesc, DepthBufferTextureSRV);

		for (int i = 0; i < 20000; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = GPUConstantBuffer->GetGPUVirtualAddress() + i * 256;
			CBVDesc.SizeInBytes = 256;

			ConstantBufferCBVs[i].ptr = ConstantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + ConstantBufferDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ConstantBufferDescriptorsCount++;

			Device->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs[i]);
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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		SIZE_T ResolvedDepthBufferTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ResolvedDepthBufferTextureOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory2)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.DepthStencil.Depth = 1.0f;
		ClearValue.DepthStencil.Stencil = 0;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory2, ResolvedDepthBufferTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(ResolvedDepthBufferTexture)));

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		ResolvedDepthBufferTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(ResolvedDepthBufferTexture, &SRVDesc, ResolvedDepthBufferTextureSRV);
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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		SIZE_T OcclusionBufferTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.SizeInBytes - HeapDesc.SizeInBytes % ResourceAllocationInfo.SizeInBytes);
		HeapDesc.SizeInBytes = OcclusionBufferTextureOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory3)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory3, OcclusionBufferTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE, &ClearValue, UUIDOF(OcclusionBufferTexture)));

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

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		SIZE_T OcclusionBufferReadbackBuffer0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = OcclusionBufferReadbackBuffer0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		SIZE_T OcclusionBufferReadbackBuffer1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = OcclusionBufferReadbackBuffer1Offset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory3)));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory3, OcclusionBufferReadbackBuffer0Offset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(OcclusionBufferTextureReadback[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory3, OcclusionBufferReadbackBuffer1Offset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(OcclusionBufferTextureReadback[1])));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		OcclusionBufferTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;

		Device->CreateRenderTargetView(OcclusionBufferTexture, &RTVDesc, OcclusionBufferTextureRTV);

		void *OcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.OcclusionBuffer");
		SIZE_T OcclusionBufferPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.OcclusionBuffer");

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

		D3D12_HEAP_DESC HeapDesc;
		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L1;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapTextureResourceDesc);

		SIZE_T CascadedShadowMapTexture0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CascadedShadowMapTexture0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapTextureResourceDesc);

		SIZE_T CascadedShadowMapTexture1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CascadedShadowMapTexture1Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapTextureResourceDesc);

		SIZE_T CascadedShadowMapTexture2Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CascadedShadowMapTexture2Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapTextureResourceDesc);

		SIZE_T CascadedShadowMapTexture3Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CascadedShadowMapTexture3Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer1Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer2Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer2Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer3Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer3Offset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory4)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.DepthStencil.Depth = 1.0f;
		ClearValue.DepthStencil.Stencil = 0;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, CascadedShadowMapTexture0Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[0])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, CascadedShadowMapTexture1Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[1])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, CascadedShadowMapTexture2Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[2])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, CascadedShadowMapTexture3Offset, &ShadowMapTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[3])));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, ConstantBuffer0Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[0])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, ConstantBuffer1Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[1])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, ConstantBuffer2Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[2])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory4, ConstantBuffer3Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[3])));

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer00Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer00Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer01Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer01Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer10Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer10Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer11Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer11Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer20Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer20Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer21Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer21Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer30Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer30Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMapPassConstantBufferResourceDesc);

		SIZE_T ConstantBuffer31Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ConstantBuffer31Offset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory4)));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer00Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[0][0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer10Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[1][0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer20Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[2][0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer30Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[3][0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer01Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[0][1])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer11Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[1][1])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer21Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[2][1])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory4, ConstantBuffer31Offset, &ShadowMapPassConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[3][1])));

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
		DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
		DSVDesc.Texture2D.MipSlice = 0;
		DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

		CascadedShadowMapTexturesDSVs[0].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DSDescriptorsCount++;
		CascadedShadowMapTexturesDSVs[1].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DSDescriptorsCount++;
		CascadedShadowMapTexturesDSVs[2].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DSDescriptorsCount++;
		CascadedShadowMapTexturesDSVs[3].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		DSDescriptorsCount++;

		Device->CreateDepthStencilView(CascadedShadowMapTextures[0], &DSVDesc, CascadedShadowMapTexturesDSVs[0]);
		Device->CreateDepthStencilView(CascadedShadowMapTextures[1], &DSVDesc, CascadedShadowMapTexturesDSVs[1]);
		Device->CreateDepthStencilView(CascadedShadowMapTextures[2], &DSVDesc, CascadedShadowMapTexturesDSVs[2]);
		Device->CreateDepthStencilView(CascadedShadowMapTextures[3], &DSVDesc, CascadedShadowMapTexturesDSVs[3]);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		CascadedShadowMapTexturesSRVs[0].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;
		CascadedShadowMapTexturesSRVs[1].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;
		CascadedShadowMapTexturesSRVs[2].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;
		CascadedShadowMapTexturesSRVs[3].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(CascadedShadowMapTextures[0], &SRVDesc, CascadedShadowMapTexturesSRVs[0]);
		Device->CreateShaderResourceView(CascadedShadowMapTextures[1], &SRVDesc, CascadedShadowMapTexturesSRVs[1]);
		Device->CreateShaderResourceView(CascadedShadowMapTextures[2], &SRVDesc, CascadedShadowMapTexturesSRVs[2]);
		Device->CreateShaderResourceView(CascadedShadowMapTextures[3], &SRVDesc, CascadedShadowMapTexturesSRVs[3]);

		for (int j = 0; j < 4; j++)
		{
			for (int i = 0; i < 20000; i++)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
				CBVDesc.BufferLocation = GPUConstantBuffers2[j]->GetGPUVirtualAddress() + i * 256;
				CBVDesc.SizeInBytes = 256;

				ConstantBufferCBVs2[j][i].ptr = ConstantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + ConstantBufferDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				ConstantBufferDescriptorsCount++;

				Device->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs2[j][i]);
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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowMaskTextureResourceDesc);

		SIZE_T ShadowMaskTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ShadowMaskTextureOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowResolveConstantBufferResourceDesc);

		SIZE_T GPUConstantBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPUConstantBufferOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory5)));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory5, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ShadowMaskTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(ShadowMaskTexture)));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory5, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ShadowResolveConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUShadowResolveConstantBuffer)));

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowResolveConstantBufferResourceDesc);

		SIZE_T CPUConstantBuffer0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUConstantBuffer0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ShadowResolveConstantBufferResourceDesc);

		SIZE_T CPUConstantBuffer1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUConstantBuffer1Offset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory5)));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory5, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ShadowResolveConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUShadowResolveConstantBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory5, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ShadowResolveConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUShadowResolveConstantBuffers[1])));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		ShadowMaskTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;

		Device->CreateRenderTargetView(ShadowMaskTexture, &RTVDesc, ShadowMaskTextureRTV);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		ShadowMaskTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(ShadowMaskTexture, &SRVDesc, ShadowMaskTextureSRV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUShadowResolveConstantBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		ShadowResolveConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateConstantBufferView(&CBVDesc, ShadowResolveConstantBufferCBV);

		void *ShadowResolvePixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.ShadowResolve");
		SIZE_T ShadowResolvePixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.ShadowResolve");

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

		D3D12_RESOURCE_DESC DeferredLightingConstantBufferResourceDesc;
		ZeroMemory(&DeferredLightingConstantBufferResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		DeferredLightingConstantBufferResourceDesc.Alignment = 0;
		DeferredLightingConstantBufferResourceDesc.DepthOrArraySize = 1;
		DeferredLightingConstantBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
		DeferredLightingConstantBufferResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
		DeferredLightingConstantBufferResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		DeferredLightingConstantBufferResourceDesc.Height = 1;
		DeferredLightingConstantBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		DeferredLightingConstantBufferResourceDesc.MipLevels = 1;
		DeferredLightingConstantBufferResourceDesc.SampleDesc.Count = 1;
		DeferredLightingConstantBufferResourceDesc.SampleDesc.Quality = 0;
		DeferredLightingConstantBufferResourceDesc.Width = 256;

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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &HDRSceneColorTextureResourceDesc);

		SIZE_T HDRSceneColorTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = HDRSceneColorTextureOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &DeferredLightingConstantBufferResourceDesc);

		SIZE_T GPUConstantBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPUConstantBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &LightClustersBufferResourceDesc);

		SIZE_T GPULightClustersBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPULightClustersBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &LightIndicesBufferResourceDesc);

		SIZE_T GPULightIndicesBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPULightIndicesBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &PointLightsBufferResourceDesc);

		SIZE_T GPUPointLightsBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPUPointLightsBufferOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory6)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory6, HDRSceneColorTextureOffset, &HDRSceneColorTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(HDRSceneColorTexture)));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory6, GPUConstantBufferOffset, &DeferredLightingConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUDeferredLightingConstantBuffer)));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory6, GPULightClustersBufferOffset, &LightClustersBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPULightClustersBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory6, GPULightIndicesBufferOffset, &LightIndicesBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPULightIndicesBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory6, GPUPointLightsBufferOffset, &PointLightsBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPUPointLightsBuffer)));

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &DeferredLightingConstantBufferResourceDesc);

		SIZE_T CPUConstantBuffer0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUConstantBuffer0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &DeferredLightingConstantBufferResourceDesc);

		SIZE_T CPUConstantBuffer1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUConstantBuffer1Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &LightClustersBufferResourceDesc);

		SIZE_T CPULightClustersBufferOffset0 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPULightClustersBufferOffset0 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &LightClustersBufferResourceDesc);

		SIZE_T CPULightClustersBufferOffset1 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPULightClustersBufferOffset1 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &LightIndicesBufferResourceDesc);

		SIZE_T CPULightIndicesBufferOffset0 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPULightIndicesBufferOffset0 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &LightIndicesBufferResourceDesc);

		SIZE_T CPULightIndicesBufferOffset1 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPULightIndicesBufferOffset1 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &PointLightsBufferResourceDesc);

		SIZE_T CPUPointLightsBufferOffset0 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUPointLightsBufferOffset0 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &PointLightsBufferResourceDesc);

		SIZE_T CPUPointLightsBufferOffset1 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUPointLightsBufferOffset1 + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory6)));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPUConstantBuffer0Offset, &DeferredLightingConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUDeferredLightingConstantBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPUConstantBuffer1Offset, &DeferredLightingConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUDeferredLightingConstantBuffers[1])));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPULightClustersBufferOffset0, &LightClustersBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightClustersBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPULightClustersBufferOffset1, &LightClustersBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightClustersBuffers[1])));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPULightIndicesBufferOffset0, &LightIndicesBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightIndicesBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPULightIndicesBufferOffset1, &LightIndicesBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightIndicesBuffers[1])));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPUPointLightsBufferOffset0, &PointLightsBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUPointLightsBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory6, CPUPointLightsBufferOffset1, &PointLightsBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUPointLightsBuffers[1])));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

		HDRSceneColorTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;

		Device->CreateRenderTargetView(HDRSceneColorTexture, &RTVDesc, HDRSceneColorTextureRTV);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

		HDRSceneColorTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(HDRSceneColorTexture, &SRVDesc, HDRSceneColorTextureSRV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUDeferredLightingConstantBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		DeferredLightingConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateConstantBufferView(&CBVDesc, DeferredLightingConstantBufferCBV);

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
		SRVDesc.Buffer.NumElements = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
		SRVDesc.Buffer.StructureByteStride = 0;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

		LightClustersBufferSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(GPULightClustersBuffer, &SRVDesc, LightClustersBufferSRV);

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
		SRVDesc.Buffer.NumElements = ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
		SRVDesc.Buffer.StructureByteStride = 0;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

		LightIndicesBufferSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(GPULightIndicesBuffer, &SRVDesc, LightIndicesBufferSRV);

		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
		SRVDesc.Buffer.NumElements = 10000;
		SRVDesc.Buffer.StructureByteStride = sizeof(PointLight);
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

		PointLightsBufferSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(GPUPointLightsBuffer, &SRVDesc, PointLightsBufferSRV);

		void *DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.DeferredLighting");
		SIZE_T DeferredLightingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.DeferredLighting");

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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SkyVertexBufferResourceDesc);

		SIZE_T SkyVertexBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SkyVertexBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SkyIndexBufferResourceDesc);

		SIZE_T SkyIndexBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SkyIndexBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SunVertexBufferResourceDesc);

		SIZE_T SunVertexBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SunVertexBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SunIndexBufferResourceDesc);

		SIZE_T SunIndexBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SunIndexBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SkyTextureResourceDesc);

		SIZE_T SkyTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SkyTextureOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SunTextureResourceDesc);

		SIZE_T SunTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SunTextureOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SkyConstantBufferResourceDesc);

		SIZE_T GPUSkyConstantBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPUSkyConstantBufferOffset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SunConstantBufferResourceDesc);

		SIZE_T GPUSunConstantBufferOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = GPUSunConstantBufferOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory7)));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SkyVertexBufferOffset, &SkyVertexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyVertexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SkyIndexBufferOffset, &SkyIndexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyIndexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, GPUSkyConstantBufferOffset, &SkyConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUSkyConstantBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SkyTextureOffset, &SkyTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyTexture)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SunVertexBufferOffset, &SunVertexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunVertexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SunIndexBufferOffset, &SunIndexBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunIndexBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, GPUSunConstantBufferOffset, &SunConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUSunConstantBuffer)));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory7, SunTextureOffset, &SunTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunTexture)));

		HeapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE;
		HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
		HeapDesc.Properties.CreationNodeMask = 0;
		HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_L0;
		HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_CUSTOM;
		HeapDesc.Properties.VisibleNodeMask = 0;
		HeapDesc.SizeInBytes = 0;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SkyConstantBufferResourceDesc);

		SIZE_T CPUSkyConstantBufferOffset0 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUSkyConstantBufferOffset0 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SkyConstantBufferResourceDesc);

		SIZE_T CPUSkyConstantBufferOffset1 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUSkyConstantBufferOffset1 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SunConstantBufferResourceDesc);

		SIZE_T CPUSunConstantBufferOffset0 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUSunConstantBufferOffset0 + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SunConstantBufferResourceDesc);

		SIZE_T CPUSunConstantBufferOffset1 = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = CPUSunConstantBufferOffset1 + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(CPUMemory7)));

		SAFE_DX(Device->CreatePlacedResource(CPUMemory7, CPUSkyConstantBufferOffset0, &SkyConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSkyConstantBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory7, CPUSkyConstantBufferOffset1, &SkyConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSkyConstantBuffers[1])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory7, CPUSunConstantBufferOffset0, &SunConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSunConstantBuffers[0])));
		SAFE_DX(Device->CreatePlacedResource(CPUMemory7, CPUSunConstantBufferOffset1, &SunConstantBufferResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSunConstantBuffers[1])));

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

		SAFE_DX(CommandAllocators[0]->Reset());
		SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

		CommandList->CopyBufferRegion(SkyVertexBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * SkyMeshVertexCount);
		CommandList->CopyBufferRegion(SkyIndexBuffer, 0, UploadBuffer, sizeof(Vertex) * SkyMeshVertexCount, sizeof(WORD) * SkyMeshIndexCount);

		D3D12_RESOURCE_BARRIER ResourceBarriers[2];
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

		CommandList->ResourceBarrier(2, ResourceBarriers);

		SAFE_DX(CommandList->Close());

		CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

		SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

		if (CopySyncFence->GetCompletedValue() != 1)
		{
			SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
			DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
		}

		SAFE_DX(CopySyncFence->Signal(0));

		SkyVertexBufferAddress = SkyVertexBuffer->GetGPUVirtualAddress();
		SkyIndexBufferAddress = SkyIndexBuffer->GetGPUVirtualAddress();

		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUSkyConstantBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		SkyConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateConstantBufferView(&CBVDesc, SkyConstantBufferCBV);

		void *SkyVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.SkyVertexShader");
		SIZE_T SkyVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.SkyVertexShader");

		void *SkyPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.SkyPixelShader");
		SIZE_T SkyPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.SkyPixelShader");

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

		ScopedMemoryBlockArray<Texel> SkyTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(2048 * 2048);

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

		SAFE_DX(CommandAllocators[0]->Reset());
		SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

		D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

		SourceTextureCopyLocation.pResource = UploadBuffer;
		SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		DestTextureCopyLocation.pResource = SkyTexture;
		DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
		DestTextureCopyLocation.SubresourceIndex = 0;

		CommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

		D3D12_RESOURCE_BARRIER ResourceBarrier;
		ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarrier.Transition.pResource = SkyTexture;
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
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		SkyTextureSRV.ptr = TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + TexturesDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		++TexturesDescriptorsCount;

		Device->CreateShaderResourceView(SkyTexture, &SRVDesc, SkyTextureSRV);

		ReadRange.Begin = 0;
		ReadRange.End = 0;

		WrittenRange.Begin = 0;
		WrittenRange.End = sizeof(Vertex) * SunMeshVertexCount + sizeof(WORD) * SunMeshIndexCount;

		SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
		memcpy((BYTE*)MappedData, SunMeshVertices, sizeof(Vertex) * SunMeshVertexCount);
		memcpy((BYTE*)MappedData + sizeof(Vertex) * SunMeshVertexCount, SunMeshIndices, sizeof(WORD) * SunMeshIndexCount);
		UploadBuffer->Unmap(0, &WrittenRange);

		SAFE_DX(CommandAllocators[0]->Reset());
		SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

		CommandList->CopyBufferRegion(SunVertexBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * SunMeshVertexCount);
		CommandList->CopyBufferRegion(SunIndexBuffer, 0, UploadBuffer, sizeof(Vertex) * SunMeshVertexCount, sizeof(WORD) * SunMeshIndexCount);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
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

		CommandList->ResourceBarrier(2, ResourceBarriers);

		SAFE_DX(CommandList->Close());

		CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

		SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

		if (CopySyncFence->GetCompletedValue() != 1)
		{
			SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
			DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
		}

		SAFE_DX(CopySyncFence->Signal(0));

		SunVertexBufferAddress = SunVertexBuffer->GetGPUVirtualAddress();
		SunIndexBufferAddress = SunIndexBuffer->GetGPUVirtualAddress();

		CBVDesc.BufferLocation = GPUSunConstantBuffer->GetGPUVirtualAddress();
		CBVDesc.SizeInBytes = 256;

		SunConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateConstantBufferView(&CBVDesc, SunConstantBufferCBV);

		void *SunVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.SunVertexShader");
		SIZE_T SunVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.SunVertexShader");

		void *SunPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.SunPixelShader");
		SIZE_T SunPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.SunPixelShader");

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

		ScopedMemoryBlockArray<Texel> SunTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(512 * 512);

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

		SAFE_DX(CommandAllocators[0]->Reset());
		SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

		SourceTextureCopyLocation.pResource = UploadBuffer;
		SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		DestTextureCopyLocation.pResource = SunTexture;
		DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
		DestTextureCopyLocation.SubresourceIndex = 0;

		CommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

		ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarrier.Transition.pResource = SunTexture;
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

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		SunTextureSRV.ptr = TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + TexturesDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		++TexturesDescriptorsCount;

		Device->CreateShaderResourceView(SunTexture, &SRVDesc, SunTextureSRV);

		void *FogPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.Fog");
		SIZE_T FogPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.Fog");

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
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC ResourceDesc;
		ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
		ResourceDesc.Alignment = 0;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		SIZE_T ResolvedHDRSceneColorTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ResolvedHDRSceneColorTextureOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory8)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory8, ResolvedHDRSceneColorTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(ResolvedHDRSceneColorTexture)));

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		ResolvedHDRSceneColorTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(ResolvedHDRSceneColorTexture, &SRVDesc, ResolvedHDRSceneColorTextureSRV);
	}

	// ===============================================================================================================

	{
		D3D12_RESOURCE_DESC SceneLuminanceTexturesResourceDescs[4];

		int Widths[4] = { 1280, 80, 5, 1 };
		int Heights[4] = { 720, 45, 3, 1 };

		for (int i = 0; i < 4; i++)
		{
			ZeroMemory(&SceneLuminanceTexturesResourceDescs[i], sizeof(D3D12_RESOURCE_DESC));
			SceneLuminanceTexturesResourceDescs[i].Alignment = 0;
			SceneLuminanceTexturesResourceDescs[i].DepthOrArraySize = 1;
			SceneLuminanceTexturesResourceDescs[i].Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			SceneLuminanceTexturesResourceDescs[i].Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
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
		AverageLuminanceTextureResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SceneLuminanceTexturesResourceDescs[0]);

		SIZE_T SceneLuminanceTexture0Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SceneLuminanceTexture0Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SceneLuminanceTexturesResourceDescs[1]);

		SIZE_T SceneLuminanceTexture1Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SceneLuminanceTexture1Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SceneLuminanceTexturesResourceDescs[2]);

		SIZE_T SceneLuminanceTexture2Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SceneLuminanceTexture2Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &SceneLuminanceTexturesResourceDescs[3]);

		SIZE_T SceneLuminanceTexture3Offset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = SceneLuminanceTexture3Offset + ResourceAllocationInfo.SizeInBytes;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &AverageLuminanceTextureResourceDesc);

		SIZE_T AverageLuminanceTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = AverageLuminanceTextureOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory9)));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory9, SceneLuminanceTexture0Offset, &SceneLuminanceTexturesResourceDescs[0], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(SceneLuminanceTextures[0])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory9, SceneLuminanceTexture1Offset, &SceneLuminanceTexturesResourceDescs[1], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(SceneLuminanceTextures[1])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory9, SceneLuminanceTexture2Offset, &SceneLuminanceTexturesResourceDescs[2], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(SceneLuminanceTextures[2])));
		SAFE_DX(Device->CreatePlacedResource(GPUMemory9, SceneLuminanceTexture3Offset, &SceneLuminanceTexturesResourceDescs[3], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(SceneLuminanceTextures[3])));

		SAFE_DX(Device->CreatePlacedResource(GPUMemory9, AverageLuminanceTextureOffset, &AverageLuminanceTextureResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, UUIDOF(AverageLuminanceTexture)));

		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
		UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		UAVDesc.Texture2D.MipSlice = 0;
		UAVDesc.Texture2D.PlaneSlice = 0;
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 4; i++)
		{
			SceneLuminanceTexturesUAVs[i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			CBSRUADescriptorsCount++;

			Device->CreateUnorderedAccessView(SceneLuminanceTextures[i], nullptr, &UAVDesc, SceneLuminanceTexturesUAVs[i]);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 4; i++)
		{
			SceneLuminanceTexturesSRVs[i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			CBSRUADescriptorsCount++;

			Device->CreateShaderResourceView(SceneLuminanceTextures[i], &SRVDesc, SceneLuminanceTexturesSRVs[i]);
		}

		UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		UAVDesc.Texture2D.MipSlice = 0;
		UAVDesc.Texture2D.PlaneSlice = 0;
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

		AverageLuminanceTextureUAV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateUnorderedAccessView(AverageLuminanceTexture, nullptr, &UAVDesc, AverageLuminanceTextureUAV);

		SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = 0;
		SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

		AverageLuminanceTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(AverageLuminanceTexture, &SRVDesc, AverageLuminanceTextureSRV);

		void *LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.LuminanceCalc");
		SIZE_T LuminanceCalcComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.LuminanceCalc");

		void *LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.LuminanceSum");
		SIZE_T LuminanceSumComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.LuminanceSum");

		void *LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.LuminanceAvg");
		SIZE_T LuminanceAvgComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.LuminanceAvg");

		D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePipelineStateDesc;
		ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
		ComputePipelineStateDesc.CS.BytecodeLength = LuminanceCalcComputeShaderByteCodeLength;
		ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceCalcComputeShaderByteCodeData;
		ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

		SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceCalcPipelineState)));

		ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
		ComputePipelineStateDesc.CS.BytecodeLength = LuminanceSumComputeShaderByteCodeLength;
		ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceSumComputeShaderByteCodeData;
		ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

		SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceSumPipelineState)));

		ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
		ComputePipelineStateDesc.CS.BytecodeLength = LuminanceAvgComputeShaderByteCodeLength;
		ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceAvgComputeShaderByteCodeData;
		ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

		SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceAvgPipelineState)));
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
			D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

			ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDescs[0][i]);

			BloomTexturesOffsets[0][i] = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
			HeapDesc.SizeInBytes = BloomTexturesOffsets[0][i] + ResourceAllocationInfo.SizeInBytes;

			ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDescs[1][i]);

			BloomTexturesOffsets[1][i] = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
			HeapDesc.SizeInBytes = BloomTexturesOffsets[1][i] + ResourceAllocationInfo.SizeInBytes;

			ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDescs[2][i]);

			BloomTexturesOffsets[2][i] = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
			HeapDesc.SizeInBytes = BloomTexturesOffsets[2][i] + ResourceAllocationInfo.SizeInBytes;
		}

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory10)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

		for (int i = 0; i < 7; i++)
		{
			SAFE_DX(Device->CreatePlacedResource(GPUMemory10, BloomTexturesOffsets[0][i], &ResourceDescs[0][i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(BloomTextures[0][i])));
			SAFE_DX(Device->CreatePlacedResource(GPUMemory10, BloomTexturesOffsets[1][i], &ResourceDescs[1][i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(BloomTextures[1][i])));
			SAFE_DX(Device->CreatePlacedResource(GPUMemory10, BloomTexturesOffsets[2][i], &ResourceDescs[2][i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(BloomTextures[2][i])));
		}

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = 0;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

		for (int i = 0; i < 7; i++)
		{
			BloomTexturesRTVs[0][i].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			RTDescriptorsCount++;
			BloomTexturesRTVs[1][i].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			RTDescriptorsCount++;
			BloomTexturesRTVs[2][i].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			RTDescriptorsCount++;

			Device->CreateRenderTargetView(BloomTextures[0][i], &RTVDesc, BloomTexturesRTVs[0][i]);
			Device->CreateRenderTargetView(BloomTextures[1][i], &RTVDesc, BloomTexturesRTVs[1][i]);
			Device->CreateRenderTargetView(BloomTextures[2][i], &RTVDesc, BloomTexturesRTVs[2][i]);
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
			BloomTexturesSRVs[0][i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			CBSRUADescriptorsCount++;
			BloomTexturesSRVs[1][i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			CBSRUADescriptorsCount++;
			BloomTexturesSRVs[2][i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			CBSRUADescriptorsCount++;

			Device->CreateShaderResourceView(BloomTextures[0][i], &SRVDesc, BloomTexturesSRVs[0][i]);
			Device->CreateShaderResourceView(BloomTextures[1][i], &SRVDesc, BloomTexturesSRVs[1][i]);
			Device->CreateShaderResourceView(BloomTextures[2][i], &SRVDesc, BloomTexturesSRVs[2][i]);
		}

		void *BrightPassPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.BrightPass");
		SIZE_T BrightPassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.BrightPass");

		void *ImageResamplePixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.ImageResample");
		SIZE_T ImageResamplePixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.ImageResample");

		void *HorizontalBlurPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.HorizontalBlur");
		SIZE_T HorizontalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.HorizontalBlur");

		void *VerticalBlurPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.VerticalBlur");
		SIZE_T VerticalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.VerticalBlur");

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

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo;

		ResourceAllocationInfo = Device->GetResourceAllocationInfo(0, 1, &ResourceDesc);

		SIZE_T ToneMappedImageTextureOffset = (HeapDesc.SizeInBytes == 0) ? 0 : HeapDesc.SizeInBytes + (ResourceAllocationInfo.Alignment - HeapDesc.SizeInBytes % ResourceAllocationInfo.Alignment);
		HeapDesc.SizeInBytes = ToneMappedImageTextureOffset + ResourceAllocationInfo.SizeInBytes;

		SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(GPUMemory11)));

		D3D12_CLEAR_VALUE ClearValue;
		ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		ClearValue.Color[0] = 0.0f;
		ClearValue.Color[1] = 0.0f;
		ClearValue.Color[2] = 0.0f;
		ClearValue.Color[3] = 0.0f;

		SAFE_DX(Device->CreatePlacedResource(GPUMemory11, ToneMappedImageTextureOffset, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE, &ClearValue, UUIDOF(ToneMappedImageTexture)));

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

		ToneMappedImageTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;

		Device->CreateRenderTargetView(ToneMappedImageTexture, &RTVDesc, ToneMappedImageTextureRTV);

		void *HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.HDRToneMapping");
		SIZE_T HDRToneMappingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.HDRToneMapping");

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
		Device->CopyDescriptorsSimple(100000, CompiledResourcesDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), ConstantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (int i = 0; i < 100000; i++)
		{
			ObjectsDescriptorTables[i] = D3D12_GPU_DESCRIPTOR_HANDLE{ CompiledResourcesDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + i * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		}

		D3D12_CPU_DESCRIPTOR_HANDLE ResourceCPUHandle{ CompiledResourcesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + 108000 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		D3D12_GPU_DESCRIPTOR_HANDLE ResourceGPUHandle{ CompiledResourcesDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + 108000 * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		UINT ResourceHandleSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		{
			UINT DestRangeSize = 1;
			UINT SourceRangeSizes[1] = { 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[1] = { ResolvedDepthBufferTextureSRV };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
			OcclusionBufferPassDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
		}

		{
			UINT DestRangeSize = 6;
			UINT SourceRangeSizes[7] = { 1, 1, 4 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[7] = { ShadowResolveConstantBufferCBV, ResolvedDepthBufferTextureSRV, CascadedShadowMapTexturesSRVs[0] };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 3, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 6 * ResourceHandleSize;
			ShadowResolvePassConstantBuffersDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			ShadowResolvePassShaderResourcesDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 5 * ResourceHandleSize;
		}

		{
			UINT DestRangeSize = 8;
			UINT SourceRangeSizes[7] = { 1, 2, 1, 1, 1, 1, 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[7] = { DeferredLightingConstantBufferCBV, GBufferTexturesSRVs[0], DepthBufferTextureSRV, ShadowMaskTextureSRV, LightClustersBufferSRV, LightIndicesBufferSRV, PointLightsBufferSRV };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 7, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 8 * ResourceHandleSize;
			DeferredLightingPassConstantBuffersDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			DeferredLightingPassShaderResourcesDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 7 * ResourceHandleSize;
		}

		{
			UINT DestRangeSize = 1;
			UINT SourceRangeSizes[2] = { 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { DepthBufferTextureSRV };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
			FogPassDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			DestRangeSize = 2;
			SourceRangeSizes[0] = 1;
			SourceRangeSizes[1] = 1;
			SourceCPUHandles[0] = SkyConstantBufferCBV;
			SourceCPUHandles[1] = SkyTextureSRV;

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			SkyPassConstantBuffersDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			SkyPassShaderResourcesDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			DestRangeSize = 2;
			SourceRangeSizes[0] = 1;
			SourceRangeSizes[1] = 1;
			SourceCPUHandles[0] = SunConstantBufferCBV;
			SourceCPUHandles[1] = SunTextureSRV;

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			SunPassConstantBuffersDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			SunPassShaderResourcesDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
		}

		{
			UINT DestRangeSize = 2;
			UINT SourceRangeSizes[2] = { 1, 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { ResolvedHDRSceneColorTextureSRV, SceneLuminanceTexturesUAVs[0] };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			SceneLuminancePassShaderResourcesDescriptorTables[0] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			SceneLuminancePassUnorderedAccessViewsDescriptorTables[0] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			DestRangeSize = 2;
			SourceRangeSizes[0] = 1;
			SourceRangeSizes[1] = 1;
			SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[0];
			SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[1];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			SceneLuminancePassShaderResourcesDescriptorTables[1] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			SceneLuminancePassUnorderedAccessViewsDescriptorTables[1] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			DestRangeSize = 2;
			SourceRangeSizes[0] = 1;
			SourceRangeSizes[1] = 1;
			SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[1];
			SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[2];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			SceneLuminancePassShaderResourcesDescriptorTables[2] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			SceneLuminancePassUnorderedAccessViewsDescriptorTables[2] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			DestRangeSize = 2;
			SourceRangeSizes[0] = 1;
			SourceRangeSizes[1] = 1;
			SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[2];
			SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[3];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			SceneLuminancePassShaderResourcesDescriptorTables[3] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			SceneLuminancePassUnorderedAccessViewsDescriptorTables[3] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			DestRangeSize = 2;
			SourceRangeSizes[0] = 1;
			SourceRangeSizes[1] = 1;
			SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[3];
			SourceCPUHandles[1] = AverageLuminanceTextureUAV;

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			SceneLuminancePassShaderResourcesDescriptorTables[4] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			SceneLuminancePassUnorderedAccessViewsDescriptorTables[4] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
		}

		{
			UINT DestRangeSize = 2;
			UINT SourceRangeSizes[2] = { 1, 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { ResolvedHDRSceneColorTextureSRV, SceneLuminanceTexturesSRVs[0] };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 2 * ResourceHandleSize;
			PostProcessBloomPassDescriptorTables[0] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

			DestRangeSize = 1;
			SourceRangeSizes[0] = 1;
			SourceCPUHandles[0] = BloomTexturesSRVs[0][0];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
			PostProcessBloomPassDescriptorTables[1] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			DestRangeSize = 1;
			SourceRangeSizes[0] = 1;
			SourceCPUHandles[0] = BloomTexturesSRVs[1][0];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
			PostProcessBloomPassDescriptorTables[2] = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			for (int i = 1; i < 7; i++)
			{
				DestRangeSize = 1;
				SourceRangeSizes[0] = 1;
				SourceCPUHandles[0] = BloomTexturesSRVs[0][i - 1];

				Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
				PostProcessBloomPassDescriptorTables[3 + (i - 1) * 3] = ResourceGPUHandle;
				ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

				DestRangeSize = 1;
				SourceRangeSizes[0] = 1;
				SourceCPUHandles[0] = BloomTexturesSRVs[0][i];

				Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
				PostProcessBloomPassDescriptorTables[3 + (i - 1) * 3 + 1] = ResourceGPUHandle;
				ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

				DestRangeSize = 1;
				SourceRangeSizes[0] = 1;
				SourceCPUHandles[0] = BloomTexturesSRVs[1][i];

				Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
				PostProcessBloomPassDescriptorTables[3 + (i - 1) * 3 + 2] = ResourceGPUHandle;
				ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			}

			for (int i = 5; i >= 0; i--)
			{
				DestRangeSize = 1;
				SourceRangeSizes[0] = 1;
				SourceCPUHandles[0] = BloomTexturesSRVs[2][i + 1];

				Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
				PostProcessBloomPassDescriptorTables[3 + 6 * 3 + (5 - i)] = ResourceGPUHandle;
				ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
			}
		}

		{
			UINT DestRangeSize = 2;
			UINT SourceRangeSizes[2] = { 1, 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { HDRSceneColorTextureSRV, BloomTexturesSRVs[2][0] };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;
			HDRToneMappingPassDescriptorTable = ResourceGPUHandle;
			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;
		}
	}

	// ===============================================================================================================

	RenderStages.Add(new GBufferOpaqueStage());
	RenderStages.Add(new MSAADepthBufferResolveStage());
	RenderStages.Add(new OcclusionBufferStage());
	RenderStages.Add(new ShadowMapStage());
	RenderStages.Add(new ShadowResolveStage());
	RenderStages.Add(new DeferredLightingStage());
	RenderStages.Add(new SkyAndFogStage());
	RenderStages.Add(new HDRSceneColorResolveStage());
	RenderStages.Add(new PostProcessLuminanceStage());
	RenderStages.Add(new PostProcessBloomStage());
	RenderStages.Add(new PostProcessHDRToneMappingStage());
	RenderStages.Add(new BackBufferResolveStage());

	for (RenderStage* renderStage : RenderStages)
	{
		renderStage->Init(&renderGraph);
	}

	renderGraph.CompileGraph();

	renderGraph.ExportGraphToHTML();

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
	ShadowViewMatrices[1] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 20.0f, CameraLocation.y + 20.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[2] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 50.0f, CameraLocation.y + 50.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[3] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 100.0f, CameraLocation.y + 100.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ShadowProjMatrices[0] = XMMatrixOrthographicLH(10.0f, 10.0f, 0.01f, 500.0f);
	ShadowProjMatrices[1] = XMMatrixOrthographicLH(20.0f, 20.0f, 0.01f, 500.0f);
	ShadowProjMatrices[2] = XMMatrixOrthographicLH(50.0f, 50.0f, 0.01f, 500.0f);
	ShadowProjMatrices[3] = XMMatrixOrthographicLH(100.0f, 100.0f, 0.01f, 500.0f);

	ShadowViewProjMatrices[0] = ShadowViewMatrices[0] * ShadowProjMatrices[0];
	ShadowViewProjMatrices[1] = ShadowViewMatrices[1] * ShadowProjMatrices[1];
	ShadowViewProjMatrices[2] = ShadowViewMatrices[2] * ShadowProjMatrices[2];
	ShadowViewProjMatrices[3] = ShadowViewMatrices[3] * ShadowProjMatrices[3];

	RenderScene& renderScene = gameFramework.GetWorld().GetRenderScene();

	DynamicArray<StaticMeshComponent*> AllStaticMeshComponents = renderScene.GetStaticMeshComponents();
	DynamicArray<StaticMeshComponent*> VisbleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.GetLength();

	DynamicArray<PointLightComponent*> AllPointLightComponents = renderScene.GetPointLightComponents();
	DynamicArray<PointLightComponent*> VisblePointLightComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

	Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().ClusterizeLights(VisblePointLightComponents, ViewMatrix);

	DynamicArray<PointLight> PointLights;

	for (PointLightComponent *pointLightComponent : VisblePointLightComponents)
	{
		PointLight pointLight;
		pointLight.Brightness = pointLightComponent->GetBrightness();
		pointLight.Color = pointLightComponent->GetColor();
		pointLight.Position = pointLightComponent->GetTransformComponent()->GetLocation();
		pointLight.Radius = pointLightComponent->GetRadius();

		PointLights.Add(pointLight);
	}

	if (FrameSyncFences[CurrentFrameIndex]->GetCompletedValue() != 1)
	{
		SAFE_DX(FrameSyncFences[CurrentFrameIndex]->SetEventOnCompletion(1, FrameSyncEvent));
		DWORD WaitResult = WaitForSingleObject(FrameSyncEvent, INFINITE);
	}

	SAFE_DX(FrameSyncFences[CurrentFrameIndex]->Signal(0));

	SAFE_DX(CommandAllocators[CurrentFrameIndex]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[CurrentFrameIndex], nullptr));

	/*D3D12_CPU_DESCRIPTOR_HANDLE ResourceCPUHandle = FrameResourcesDescriptorHeaps[CurrentFrameIndex]->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE ResourceGPUHandle = FrameResourcesDescriptorHeaps[CurrentFrameIndex]->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE SamplerCPUHandle = FrameSamplersDescriptorHeaps[CurrentFrameIndex]->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE SamplerGPUHandle = FrameSamplersDescriptorHeaps[CurrentFrameIndex]->GetGPUDescriptorHandleForHeapStart();
	UINT ResourceHandleSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	UINT SamplerHandleSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);*/

	ID3D12DescriptorHeap *DescriptorHeaps[2] = { CompiledResourcesDescriptorHeap, CompiledSamplersDescriptorHeap };

	CommandList->SetDescriptorHeaps(2, DescriptorHeaps);
	CommandList->SetGraphicsRootSignature(GraphicsRootSignature);
	CommandList->SetComputeRootSignature(ComputeRootSignature);

	OPTICK_EVENT("Draw Calls")

	// ===============================================================================================================

	D3D12_RESOURCE_BARRIER ResourceBarriers[8];
	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = GBufferTextures[0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = GBufferTextures[1];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;
		SIZE_T ConstantBufferOffset = 0;

		SAFE_DX(CPUConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			GBufferOpaquePassConstantBuffer& ConstantBuffer = *((GBufferOpaquePassConstantBuffer*)((BYTE*)ConstantBufferData + ConstantBufferOffset));

			XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
			XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;

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

			ConstantBuffer.WVPMatrix = WVPMatrix;
			ConstantBuffer.WorldMatrix = WorldMatrix;
			ConstantBuffer.VectorTransformMatrix = VectorTransformMatrix;

			ConstantBufferOffset += 256;
		}

		WrittenRange.Begin = 0;
		WrittenRange.End = ConstantBufferOffset;

		CPUConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[2].Transition.pResource = GPUConstantBuffer;
		ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[2].Transition.Subresource = 0;
		ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(3, ResourceBarriers);

		CommandList->CopyBufferRegion(GPUConstantBuffer, 0, CPUConstantBuffers[CurrentFrameIndex], 0, ConstantBufferOffset);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		CommandList->OMSetRenderTargets(2, GBufferTexturesRTVs, TRUE, &DepthBufferTextureDSV);

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

		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		CommandList->ClearRenderTargetView(GBufferTexturesRTVs[0], ClearColor, 0, nullptr);
		CommandList->ClearRenderTargetView(GBufferTexturesRTVs[1], ClearColor, 0, nullptr);
		CommandList->ClearDepthStencilView(DepthBufferTextureDSV, D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);

		CommandList->SetGraphicsRootDescriptorTable(5, TextureSamplerTable);

		for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

			RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
			RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
			MaterialResource *material = staticMeshComponent->GetMaterial();
			RenderTexture *renderTexture0 = material->GetTexture(0)->GetRenderTexture();
			RenderTexture *renderTexture1 = material->GetTexture(1)->GetRenderTexture();

			D3D12_VERTEX_BUFFER_VIEW VertexBufferViews[3];
			VertexBufferViews[0].BufferLocation = renderMesh->VertexBufferAddresses[0];
			VertexBufferViews[0].SizeInBytes = sizeof(XMFLOAT3) * 9 * 9 * 6;
			VertexBufferViews[0].StrideInBytes = sizeof(XMFLOAT3);
			VertexBufferViews[1].BufferLocation = renderMesh->VertexBufferAddresses[1];
			VertexBufferViews[1].SizeInBytes = sizeof(XMFLOAT2) * 9 * 9 * 6;
			VertexBufferViews[1].StrideInBytes = sizeof(XMFLOAT2);
			VertexBufferViews[2].BufferLocation = renderMesh->VertexBufferAddresses[2];
			VertexBufferViews[2].SizeInBytes = 3 * sizeof(XMFLOAT3) * 9 * 9 * 6;
			VertexBufferViews[2].StrideInBytes = 3 * sizeof(XMFLOAT3);

			D3D12_INDEX_BUFFER_VIEW IndexBufferView;
			IndexBufferView.BufferLocation = renderMesh->IndexBufferAddress;
			IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
			IndexBufferView.SizeInBytes = sizeof(WORD) * 8 * 8 * 6 * 6;

			CommandList->IASetVertexBuffers(0, 3, VertexBufferViews);
			CommandList->IASetIndexBuffer(&IndexBufferView);

			CommandList->SetPipelineState(renderMaterial->GBufferOpaquePassPipelineState);

			CommandList->SetGraphicsRootDescriptorTable(0, ObjectsDescriptorTables[k]);
			CommandList->SetGraphicsRootDescriptorTable(4, renderMaterial->MaterialDescriptorTable);

			CommandList->DrawIndexedInstanced(8 * 8 * 6 * 6, 1, 0, 0, 0);
		}
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = DepthBufferTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ResolvedDepthBufferTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(2, ResourceBarriers);

		COMRCPtr<ID3D12GraphicsCommandList1> CommandList1;

		CommandList->QueryInterface<ID3D12GraphicsCommandList1>(&CommandList1);

		CommandList1->ResolveSubresourceRegion(ResolvedDepthBufferTexture, 0, 0, 0, DepthBufferTexture, 0, nullptr, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT, D3D12_RESOLVE_MODE::D3D12_RESOLVE_MODE_MAX);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ResolvedDepthBufferTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = OcclusionBufferTexture;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &OcclusionBufferTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = 144.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = 256.0f;

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = 144;
		ScissorRect.left = 0;
		ScissorRect.right = 256;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(OcclusionBufferTexture, nullptr);

		CommandList->SetPipelineState(OcclusionBufferPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(5, MinSamplerTable);
		CommandList->SetGraphicsRootDescriptorTable(4, OcclusionBufferPassDescriptorTable);

		CommandList->DrawInstanced(4, 1, 0, 0);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = OcclusionBufferTexture;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		D3D12_RESOURCE_DESC ResourceDesc = OcclusionBufferTexture->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

		UINT NumRows;
		UINT64 RowSizeInBytes, TotalBytes;

		Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

		D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

		SourceTextureCopyLocation.pResource = OcclusionBufferTexture;
		SourceTextureCopyLocation.SubresourceIndex = 0;
		SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		DestTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
		DestTextureCopyLocation.pResource = OcclusionBufferTextureReadback[CurrentFrameIndex];
		DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		CommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

		float *OcclusionBufferData = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetOcclusionBufferData();

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = TotalBytes;

		WrittenRange.Begin = 0;
		WrittenRange.End = 0;

		void *MappedData;

		OcclusionBufferTextureReadback[CurrentFrameIndex]->Map(0, &ReadRange, &MappedData);

		for (UINT i = 0; i < NumRows; i++)
		{
			memcpy((BYTE*)OcclusionBufferData + i * RowSizeInBytes, (BYTE*)MappedData + i * PlacedSubResourceFootPrint.Footprint.RowPitch, RowSizeInBytes);
		}

		OcclusionBufferTextureReadback[CurrentFrameIndex]->Unmap(0, &WrittenRange);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = CascadedShadowMapTextures[0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = CascadedShadowMapTextures[1];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = CascadedShadowMapTextures[2];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[3].Transition.pResource = CascadedShadowMapTextures[3];
	ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[3].Transition.Subresource = 0;
	ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		for (int i = 0; i < 4; i++)
		{
			SIZE_T ConstantBufferOffset = 0;

			DynamicArray<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
			DynamicArray<StaticMeshComponent*> VisbleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i], false);
			size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.GetLength();

			D3D12_RANGE ReadRange, WrittenRange;
			ReadRange.Begin = 0;
			ReadRange.End = 0;

			void *ConstantBufferData;

			SAFE_DX(CPUConstantBuffers2[i][CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

			for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
			{
				XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
				XMMATRIX WVPMatrix = WorldMatrix * ShadowViewProjMatrices[i];

				ShadowMapPassConstantBuffer& ConstantBuffer = *((ShadowMapPassConstantBuffer*)((BYTE*)ConstantBufferData + ConstantBufferOffset));

				ConstantBuffer.WVPMatrix = WVPMatrix;

				ConstantBufferOffset += 256;
			}

			WrittenRange.Begin = 0;
			WrittenRange.End = ConstantBufferOffset;

			CPUConstantBuffers2[i][CurrentFrameIndex]->Unmap(0, &WrittenRange);

			if (i == 0)
			{
				ResourceBarriers[4].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				ResourceBarriers[4].Transition.pResource = GPUConstantBuffers2[i];
				ResourceBarriers[4].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
				ResourceBarriers[4].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
				ResourceBarriers[4].Transition.Subresource = 0;
				ResourceBarriers[4].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

				CommandList->ResourceBarrier(5, ResourceBarriers);
			}
			else
			{
				ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
				ResourceBarriers[0].Transition.pResource = GPUConstantBuffers2[i];
				ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
				ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
				ResourceBarriers[0].Transition.Subresource = 0;
				ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

				CommandList->ResourceBarrier(1, ResourceBarriers);
			}

			CommandList->CopyBufferRegion(GPUConstantBuffers2[i], 0, CPUConstantBuffers2[i][CurrentFrameIndex], 0, ConstantBufferOffset);

			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = GPUConstantBuffers2[i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(1, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			CommandList->OMSetRenderTargets(0, nullptr, TRUE, &CascadedShadowMapTexturesDSVs[i]);

			D3D12_VIEWPORT Viewport;
			Viewport.Height = 2048.0f;
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = 2048.0f;

			CommandList->RSSetViewports(1, &Viewport);

			D3D12_RECT ScissorRect;
			ScissorRect.bottom = 2048;
			ScissorRect.left = 0;
			ScissorRect.right = 2048;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			CommandList->ClearDepthStencilView(CascadedShadowMapTexturesDSVs[i], D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
			{
				StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

				RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
				RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
				RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
				RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

				UINT DestRangeSize = 1;
				UINT SourceRangeSizes[1] = { 1 };
				D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[1] = { ConstantBufferCBVs2[i][k] };

				D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
				VertexBufferView.BufferLocation = renderMesh->VertexBufferAddresses[0];
				VertexBufferView.SizeInBytes = sizeof(XMFLOAT3) * 9 * 9 * 6;
				VertexBufferView.StrideInBytes = sizeof(XMFLOAT3);

				D3D12_INDEX_BUFFER_VIEW IndexBufferView;
				IndexBufferView.BufferLocation = renderMesh->IndexBufferAddress;
				IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
				IndexBufferView.SizeInBytes = sizeof(WORD) * 8 * 8 * 6 * 6;

				CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
				CommandList->IASetIndexBuffer(&IndexBufferView);

				CommandList->SetPipelineState(renderMaterial->ShadowMapPassPipelineState);

				CommandList->SetGraphicsRootDescriptorTable(0, ObjectsDescriptorTables[(i + 1) * 20000 + k]);

				CommandList->DrawIndexedInstanced(8 * 8 * 6 * 6, 1, 0, 0, 0);
			}
		}
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ShadowMaskTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = CascadedShadowMapTextures[0];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = CascadedShadowMapTextures[1];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[3].Transition.pResource = CascadedShadowMapTextures[2];
	ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[3].Transition.Subresource = 0;
	ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[4].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[4].Transition.pResource = CascadedShadowMapTextures[3];
	ResourceBarriers[4].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[4].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[4].Transition.Subresource = 0;
	ResourceBarriers[4].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

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

		SAFE_DX(CPUShadowResolveConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		ShadowResolveConstantBuffer& ConstantBuffer = *((ShadowResolveConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.ReProjMatrices[0] = ReProjMatrices[0];
		ConstantBuffer.ReProjMatrices[1] = ReProjMatrices[1];
		ConstantBuffer.ReProjMatrices[2] = ReProjMatrices[2];
		ConstantBuffer.ReProjMatrices[3] = ReProjMatrices[3];

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUShadowResolveConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		ResourceBarriers[5].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[5].Transition.pResource = GPUShadowResolveConstantBuffer;
		ResourceBarriers[5].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[5].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[5].Transition.Subresource = 0;
		ResourceBarriers[5].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(6, ResourceBarriers);

		CommandList->CopyBufferRegion(GPUShadowResolveConstantBuffer, 0, CPUShadowResolveConstantBuffers[CurrentFrameIndex], 0, 256);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUShadowResolveConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &ShadowMaskTextureRTV, TRUE, nullptr);

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

		CommandList->DiscardResource(ShadowMaskTexture, nullptr);

		CommandList->SetPipelineState(ShadowResolvePipelineState);

		CommandList->SetGraphicsRootDescriptorTable(3, ShadowResolvePassConstantBuffersDescriptorTable);
		CommandList->SetGraphicsRootDescriptorTable(4, ShadowResolvePassShaderResourcesDescriptorTable);
		CommandList->SetGraphicsRootDescriptorTable(5, ShadowMapSamplerTable);

		CommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = GBufferTextures[0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = GBufferTextures[1];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = HDRSceneColorTexture;
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[3].Transition.pResource = ShadowMaskTexture;
	ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[3].Transition.Subresource = 0;
	ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUDeferredLightingConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

		DeferredLightingConstantBuffer& ConstantBuffer = *((DeferredLightingConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.InvViewProjMatrix = InvViewProjMatrix;
		ConstantBuffer.CameraWorldPosition = CameraLocation;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUDeferredLightingConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		void *DynamicBufferData;

		SAFE_DX(CPULightClustersBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

		WrittenRange.Begin = 0;
		WrittenRange.End = 32 * 18 * 24 * 2 * sizeof(uint32_t);

		CPULightClustersBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		SAFE_DX(CPULightIndicesBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetLightIndicesData(), Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t));

		WrittenRange.Begin = 0;
		WrittenRange.End = Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t);

		CPULightIndicesBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		SAFE_DX(CPUPointLightsBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, PointLights.GetData(), PointLights.GetLength() * sizeof(PointLight));

		WrittenRange.Begin = 0;
		WrittenRange.End = PointLights.GetLength() * sizeof(PointLight);

		CPUPointLightsBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		ResourceBarriers[4].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[4].Transition.pResource = GPUDeferredLightingConstantBuffer;
		ResourceBarriers[4].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[4].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[4].Transition.Subresource = 0;
		ResourceBarriers[4].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[5].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[5].Transition.pResource = GPULightClustersBuffer;
		ResourceBarriers[5].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[5].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[5].Transition.Subresource = 0;
		ResourceBarriers[5].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[6].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[6].Transition.pResource = GPULightIndicesBuffer;
		ResourceBarriers[6].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[6].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[6].Transition.Subresource = 0;
		ResourceBarriers[6].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[7].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[7].Transition.pResource = GPUPointLightsBuffer;
		ResourceBarriers[7].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[7].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[7].Transition.Subresource = 0;
		ResourceBarriers[7].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(8, ResourceBarriers);

		CommandList->CopyBufferRegion(GPUDeferredLightingConstantBuffer, 0, CPUDeferredLightingConstantBuffers[CurrentFrameIndex], 0, 256);
		CommandList->CopyBufferRegion(GPULightClustersBuffer, 0, CPULightClustersBuffers[CurrentFrameIndex], 0, ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * sizeof(LightCluster));
		CommandList->CopyBufferRegion(GPULightIndicesBuffer, 0, CPULightIndicesBuffers[CurrentFrameIndex], 0, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t));
		CommandList->CopyBufferRegion(GPUPointLightsBuffer, 0, CPUPointLightsBuffers[CurrentFrameIndex], 0, PointLights.GetLength() * sizeof(PointLight));

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUDeferredLightingConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = GPULightClustersBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[2].Transition.pResource = GPULightIndicesBuffer;
		ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[2].Transition.Subresource = 0;
		ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[3].Transition.pResource = GPUPointLightsBuffer;
		ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[3].Transition.Subresource = 0;
		ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(4, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

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

		CommandList->DiscardResource(HDRSceneColorTexture, nullptr);

		CommandList->SetPipelineState(DeferredLightingPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(3, DeferredLightingPassConstantBuffersDescriptorTable);
		CommandList->SetGraphicsRootDescriptorTable(4, DeferredLightingPassShaderResourcesDescriptorTable);

		CommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	{
		XMMATRIX SkyWorldMatrix = XMMatrixScaling(900.0f, 900.0f, 900.0f) * XMMatrixTranslation(CameraLocation.x, CameraLocation.y, CameraLocation.z);
		XMMATRIX SkyWVPMatrix = SkyWorldMatrix * ViewProjMatrix;

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUSkyConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		SkyConstantBuffer& skyConstantBuffer = *((SkyConstantBuffer*)((BYTE*)ConstantBufferData));

		skyConstantBuffer.WVPMatrix = SkyWVPMatrix;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUSkyConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);

		SAFE_DX(CPUSunConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		SunConstantBuffer& sunConstantBuffer = *((SunConstantBuffer*)((BYTE*)ConstantBufferData));

		sunConstantBuffer.ViewMatrix = ViewMatrix;
		sunConstantBuffer.ProjMatrix = ProjMatrix;
		sunConstantBuffer.SunPosition = SunPosition;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUSunConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUSkyConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = GPUSunConstantBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->CopyBufferRegion(GPUSkyConstantBuffer, 0, CPUSkyConstantBuffers[CurrentFrameIndex], 0, 256);
		CommandList->CopyBufferRegion(GPUSunConstantBuffer, 0, CPUSunConstantBuffers[CurrentFrameIndex], 0, 256);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUSkyConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = GPUSunConstantBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

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

		CommandList->SetPipelineState(FogPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, FogPassDescriptorTable);

		CommandList->DrawInstanced(4, 1, 0, 0);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = DepthBufferTexture;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		CommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, &DepthBufferTextureDSV);

		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->SetGraphicsRootDescriptorTable(5, TextureSamplerTable);

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		VertexBufferView.BufferLocation = SkyVertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * (1 + 25 * 100 + 1);
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		IndexBufferView.BufferLocation = SkyIndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * (300 + 24 * 600 + 300);

		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		CommandList->IASetIndexBuffer(&IndexBufferView);

		CommandList->SetPipelineState(SkyPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(0, SkyPassConstantBuffersDescriptorTable);
		CommandList->SetGraphicsRootDescriptorTable(4, SkyPassShaderResourcesDescriptorTable);

		CommandList->DrawIndexedInstanced(300 + 24 * 600 + 300, 1, 0, 0, 0);

		VertexBufferView.BufferLocation = SunVertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * 4;
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		IndexBufferView.BufferLocation = SunIndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * 6;

		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		CommandList->IASetIndexBuffer(&IndexBufferView);

		CommandList->SetPipelineState(SunPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(0, SunPassConstantBuffersDescriptorTable);
		CommandList->SetGraphicsRootDescriptorTable(4, SunPassShaderResourcesDescriptorTable);

		CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = HDRSceneColorTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ResolvedHDRSceneColorTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->ResolveSubresource(ResolvedHDRSceneColorTexture, 0, HDRSceneColorTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = HDRSceneColorTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ResolvedHDRSceneColorTexture;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = SceneLuminanceTextures[0];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(3, ResourceBarriers);

		CommandList->SetPipelineState(LuminanceCalcPipelineState);

		CommandList->SetComputeRootDescriptorTable(1, SceneLuminancePassShaderResourcesDescriptorTables[0]);
		CommandList->SetComputeRootDescriptorTable(3, SceneLuminancePassUnorderedAccessViewsDescriptorTables[0]);

		CommandList->DiscardResource(SceneLuminanceTextures[0], nullptr);

		CommandList->Dispatch(80, 45, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[1];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->SetPipelineState(LuminanceSumPipelineState);

		CommandList->SetComputeRootDescriptorTable(1, SceneLuminancePassShaderResourcesDescriptorTables[1]);
		CommandList->SetComputeRootDescriptorTable(3, SceneLuminancePassUnorderedAccessViewsDescriptorTables[1]);

		CommandList->DiscardResource(SceneLuminanceTextures[1], nullptr);

		CommandList->Dispatch(80, 45, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[1];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[2];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->SetComputeRootDescriptorTable(1, SceneLuminancePassShaderResourcesDescriptorTables[2]);
		CommandList->SetComputeRootDescriptorTable(3, SceneLuminancePassUnorderedAccessViewsDescriptorTables[2]);

		CommandList->DiscardResource(SceneLuminanceTextures[2], nullptr);

		CommandList->Dispatch(5, 3, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[2];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[3];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->SetComputeRootDescriptorTable(1, SceneLuminancePassShaderResourcesDescriptorTables[3]);
		CommandList->SetComputeRootDescriptorTable(3, SceneLuminancePassUnorderedAccessViewsDescriptorTables[3]);

		CommandList->DiscardResource(SceneLuminanceTextures[3], nullptr);

		CommandList->Dispatch(1, 1, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[3];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->SetPipelineState(LuminanceAvgPipelineState);

		CommandList->SetComputeRootDescriptorTable(1, SceneLuminancePassShaderResourcesDescriptorTables[4]);
		CommandList->SetComputeRootDescriptorTable(3, SceneLuminancePassUnorderedAccessViewsDescriptorTables[4]);

		CommandList->DiscardResource(AverageLuminanceTexture, nullptr);

		CommandList->Dispatch(1, 1, 1);
	}

	// ===============================================================================================================

	{
		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = BloomTextures[0][0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[0][0], TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(BloomTextures[0][0], nullptr);

		CommandList->SetPipelineState(BrightPassPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, PostProcessBloomPassDescriptorTables[0]);

		CommandList->DrawInstanced(4, 1, 0, 0);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = BloomTextures[0][0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = BloomTextures[1][0];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[1][0], TRUE, nullptr);

		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(BloomTextures[1][0], nullptr);		

		CommandList->SetPipelineState(HorizontalBlurPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, PostProcessBloomPassDescriptorTables[1]);

		CommandList->DrawInstanced(4, 1, 0, 0);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = BloomTextures[1][0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = BloomTextures[2][0];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][0], TRUE, nullptr);

		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(BloomTextures[2][0], nullptr);

		CommandList->SetPipelineState(VerticalBlurPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, PostProcessBloomPassDescriptorTables[2]);

		CommandList->DrawInstanced(4, 1, 0, 0);

		for (int i = 1; i < 7; i++)
		{
			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[0][i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(1, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[0][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			CommandList->DiscardResource(BloomTextures[0][i], nullptr);

			CommandList->SetPipelineState(DownSamplePipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, PostProcessBloomPassDescriptorTables[3 + (i - 1) * 3]);

			CommandList->DrawInstanced(4, 1, 0, 0);

			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[0][i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[1].Transition.pResource = BloomTextures[1][i];
			ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[1].Transition.Subresource = 0;
			ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(2, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[1][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			CommandList->DiscardResource(BloomTextures[1][i], nullptr);

			CommandList->SetPipelineState(HorizontalBlurPipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, PostProcessBloomPassDescriptorTables[3 + (i - 1) * 3 + 1]);

			CommandList->DrawInstanced(4, 1, 0, 0);

			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[1][i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[1].Transition.pResource = BloomTextures[2][i];
			ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[1].Transition.Subresource = 0;
			ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(2, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			CommandList->DiscardResource(BloomTextures[2][i], nullptr);			

			CommandList->SetPipelineState(VerticalBlurPipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, PostProcessBloomPassDescriptorTables[3 + (i - 1) * 3 + 2]);

			CommandList->DrawInstanced(4, 1, 0, 0);
		}

		for (int i = 5; i >= 0; i--)
		{
			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[2][i + 1];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(1, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);			

			CommandList->SetPipelineState(UpSampleWithAddBlendPipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, PostProcessBloomPassDescriptorTables[3 + 6 * 3 + (5 - i)]);

			CommandList->DrawInstanced(4, 1, 0, 0);
		}
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = BloomTextures[2][0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ToneMappedImageTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &ToneMappedImageTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(ToneMappedImageTexture, nullptr);

		CommandList->SetPipelineState(HDRToneMappingPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, HDRToneMappingPassDescriptorTable);

		CommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ToneMappedImageTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = BackBufferTextures[CurrentBackBufferIndex];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->ResolveSubresource(BackBufferTextures[CurrentBackBufferIndex], 0, ToneMappedImageTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = BackBufferTextures[CurrentBackBufferIndex];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, ResourceBarriers);

	// ===============================================================================================================

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));

	SAFE_DX(CommandQueue->Signal(FrameSyncFences[CurrentFrameIndex], 1));

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	for (RenderStage* renderStage : RenderStages)
	{
		renderStage->Execute();
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

	renderTexture->TextureSRV.ptr = TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + TexturesDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	++TexturesDescriptorsCount;

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
	GraphicsPipelineStateDesc.NumRenderTargets = 2;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.RTVFormats[1] = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
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

	D3D12_CPU_DESCRIPTOR_HANDLE DestHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ CompiledResourcesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + (100000 + ResourcesDescriptorsCount) * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	Device->CopyDescriptorsSimple(1, DestHandle, renderMaterialCreateInfo.Textures[0]->GetRenderTexture()->TextureSRV, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DestHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ CompiledResourcesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + (ResourcesDescriptorsCount + 100000 + 1) * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	Device->CopyDescriptorsSimple(1, DestHandle, renderMaterialCreateInfo.Textures[1]->GetRenderTexture()->TextureSRV, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	static int MaterialsDescriptorTablesCount = 0;

	MaterialsDescriptorTables[MaterialsDescriptorTablesCount] = D3D12_GPU_DESCRIPTOR_HANDLE{ CompiledResourcesDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + (100000 + ResourcesDescriptorsCount) * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
	ResourcesDescriptorsCount += 2;
		
	renderMaterial->MaterialDescriptorTable = MaterialsDescriptorTables[MaterialsDescriptorTablesCount];
	MaterialsDescriptorTablesCount++;

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

		const char16_t *DXErrorCodePtr = GetDXErrorMessageFromHRESULT(hr);

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