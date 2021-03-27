// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "DeferredLightingPass.h"

#include "../ClusterizationSubSystem.h"

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

struct DeferredLightingConstantBuffer
{
	XMMATRIX InvViewProjMatrix;
	XMFLOAT3 CameraWorldPosition;
};

void DeferredLightingPass::Init(RenderSystem& renderSystem)
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
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(HDRSceneColorTexture)));

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
	ResourceDesc.Width = 256;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUDeferredLightingConstantBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUDeferredLightingConstantBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUDeferredLightingConstantBuffers[1])));

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUDeferredLightingConstantBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	DeferredLightingConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateConstantBufferView(&CBVDesc, DeferredLightingConstantBufferCBV);

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
	ResourceDesc.Width = sizeof(LightCluster) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPULightClustersBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightClustersBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightClustersBuffers[1])));

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
	ResourceDesc.Width = ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPULightIndicesBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightIndicesBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightIndicesBuffers[1])));

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
	ResourceDesc.Width = 10000 * sizeof(PointLight);

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPUPointLightsBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUPointLightsBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUPointLightsBuffers[1])));

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

	HANDLE DeferredLightingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/DeferredLighting.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER DeferredLightingPixelShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(DeferredLightingPixelShaderFile, &DeferredLightingPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(DeferredLightingPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(DeferredLightingPixelShaderFile, DeferredLightingPixelShaderByteCodeData, (DWORD)DeferredLightingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(DeferredLightingPixelShaderFile);

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
	GraphicsPipelineStateDesc.PS.BytecodeLength = DeferredLightingPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = DeferredLightingPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DeferredLightingPipelineState)));
}

void DeferredLightingPass::Execute(RenderSystem& renderSystem)
{
	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	RenderScene& renderScene = gameFramework.GetWorld().GetRenderScene();

	Camera& camera = gameFramework.GetCamera();

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();

	vector<PointLightComponent*> AllPointLightComponents = renderScene.GetPointLightComponents();
	vector<PointLightComponent*> VisblePointLightComponents = cullingSubSystem.GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

	clusterizationSubSystem.ClusterizeLights(VisblePointLightComponents, ViewMatrix);

	vector<PointLight> PointLights;

	for (PointLightComponent *pointLightComponent : VisblePointLightComponents)
	{
		PointLight pointLight;
		pointLight.Brightness = pointLightComponent->GetBrightness();
		pointLight.Color = pointLightComponent->GetColor();
		pointLight.Position = pointLightComponent->GetTransformComponent()->GetLocation();
		pointLight.Radius = pointLightComponent->GetRadius();

		PointLights.push_back(pointLight);
	}

	D3D12_RESOURCE_BARRIER ResourceBarriers[4];

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

	memcpy(DynamicBufferData, clusterizationSubSystem.GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

	WrittenRange.Begin = 0;
	WrittenRange.End = 32 * 18 * 24 * 2 * sizeof(uint32_t);

	CPULightClustersBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

	SAFE_DX(CPULightIndicesBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

	memcpy(DynamicBufferData, clusterizationSubSystem.GetLightIndicesData(), clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t));

	WrittenRange.Begin = 0;
	WrittenRange.End = clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t);

	CPULightIndicesBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

	SAFE_DX(CPUPointLightsBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

	memcpy(DynamicBufferData, PointLights.data(), PointLights.size() * sizeof(PointLight));

	WrittenRange.Begin = 0;
	WrittenRange.End = PointLights.size() * sizeof(PointLight);

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

	renderSystem.GetCommandList()->ResourceBarrier(8, ResourceBarriers);

	renderSystem.GetCommandList()->CopyBufferRegion(GPUDeferredLightingConstantBuffer, 0, CPUDeferredLightingConstantBuffers[CurrentFrameIndex], 0, 256);
	renderSystem.GetCommandList()->CopyBufferRegion(GPULightClustersBuffer, 0, CPULightClustersBuffers[CurrentFrameIndex], 0, ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * sizeof(LightCluster));
	renderSystem.GetCommandList()->CopyBufferRegion(GPULightIndicesBuffer, 0, CPULightIndicesBuffers[CurrentFrameIndex], 0, clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t));
	renderSystem.GetCommandList()->CopyBufferRegion(GPUPointLightsBuffer, 0, CPUPointLightsBuffers[CurrentFrameIndex], 0, PointLights.size() * sizeof(PointLight));

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

	renderSystem.GetCommandList()->ResourceBarrier(4, ResourceBarriers);

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = float(ResolutionHeight);
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(ResolutionWidth);

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = ResolutionHeight;
	ScissorRect.left = 0;
	ScissorRect.right = ResolutionWidth;
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderSystem.GetCommandList()->DiscardResource(HDRSceneColorTexture, nullptr);

	UINT DestRangeSize = 8;
	UINT SourceRangeSizes[7] = { 1, 2, 1, 1, 1, 1, 1 };
	D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[7] = { DeferredLightingConstantBufferCBV, GBufferTexturesSRVs[0], DepthBufferTextureSRV, ShadowMaskTextureSRV, LightClustersBufferSRV, LightIndicesBufferSRV, PointLightsBufferSRV };

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 7, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 8 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetPipelineState(DeferredLightingPipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

	ResourceGPUHandle.ptr += 8 * ResourceHandleSize;

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);
}