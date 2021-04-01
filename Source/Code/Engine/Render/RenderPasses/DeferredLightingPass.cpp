// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "DeferredLightingPass.h"

#include "GBufferOpaquePass.h"
#include "ShadowResolvePass.h"

#include "../ClusterizationSubSystem.h"

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

#undef SAFE_DX
#define SAFE_DX(Func) Func

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
	GBufferTextures[0] = ((GBufferOpaquePass*)renderSystem.GetRenderPass("GBufferOpaquePass"))->GetGBufferTexture(0);
	GBufferTextures[1] = ((GBufferOpaquePass*)renderSystem.GetRenderPass("GBufferOpaquePass"))->GetGBufferTexture(1);
	ShadowMaskTexture = ((ShadowResolvePass*)renderSystem.GetRenderPass("ShadowResolvePass"))->GetShadowMaskTexture();

	GBufferTexturesSRVs[0] = ((GBufferOpaquePass*)renderSystem.GetRenderPass("GBufferOpaquePass"))->GetGBufferTextureSRV(0);
	GBufferTexturesSRVs[1] = ((GBufferOpaquePass*)renderSystem.GetRenderPass("GBufferOpaquePass"))->GetGBufferTextureSRV(1);
	DepthBufferTextureSRV = ((GBufferOpaquePass*)renderSystem.GetRenderPass("GBufferOpaquePass"))->GetDepthBufferTextureSRV();
	ShadowMaskTextureSRV = ((ShadowResolvePass*)renderSystem.GetRenderPass("ShadowResolvePass"))->GetShadowMaskTextureSRV();

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	HDRSceneColorTexture = renderSystem.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderSystem.GetResolutionWidth(), renderSystem.GetResolutionHeight(), DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 1, 8), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	HDRSceneColorTextureRTV = renderSystem.GetRTDescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateRenderTargetView(HDRSceneColorTexture.DXTexture, &RTVDesc, HDRSceneColorTextureRTV);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	HDRSceneColorTextureSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(HDRSceneColorTexture.DXTexture, &SRVDesc, HDRSceneColorTextureSRV);

	GPUDeferredLightingConstantBuffer = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);

	CPUDeferredLightingConstantBuffers[0] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUDeferredLightingConstantBuffers[1] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUDeferredLightingConstantBuffer.DXBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	DeferredLightingConstantBufferCBV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateConstantBufferView(&CBVDesc, DeferredLightingConstantBufferCBV);

	GPULightClustersBuffer = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(sizeof(LightCluster) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr);

	CPULightClustersBuffers[0] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(sizeof(LightCluster) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPULightClustersBuffers[1] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(sizeof(LightCluster) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
	SRVDesc.Buffer.NumElements = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
	SRVDesc.Buffer.StructureByteStride = 0;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

	LightClustersBufferSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(GPULightClustersBuffer.DXBuffer, &SRVDesc, LightClustersBufferSRV);

	GPULightIndicesBuffer = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr);

	CPULightIndicesBuffers[0] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPULightIndicesBuffers[1] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
	SRVDesc.Buffer.NumElements = ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
	SRVDesc.Buffer.StructureByteStride = 0;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

	LightIndicesBufferSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(GPULightIndicesBuffer.DXBuffer, &SRVDesc, LightIndicesBufferSRV);

	GPUPointLightsBuffer = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(10000 * sizeof(PointLight)), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr);

	CPUPointLightsBuffers[0] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(10000 * sizeof(PointLight)), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUPointLightsBuffers[1] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(10000 * sizeof(PointLight)), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
	SRVDesc.Buffer.NumElements = 10000;
	SRVDesc.Buffer.StructureByteStride = sizeof(PointLight);
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

	PointLightsBufferSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(GPUPointLightsBuffer.DXBuffer, &SRVDesc, PointLightsBufferSRV);

	SIZE_T DeferredLightingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("DeferredLighting");
	ScopedMemoryBlockArray<BYTE> DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(DeferredLightingPixelShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("DeferredLighting", DeferredLightingPixelShaderByteCodeData);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = DeferredLightingPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = DeferredLightingPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DeferredLightingPipelineState)));

	DeferredLightingCBTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_CONSTANT_BUFFERS]);
	DeferredLightingSRTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);
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
	vector<PointLightComponent*> VisblePointLightComponents = renderSystem.GetCullingSubSystem().GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

	renderSystem.GetClusterizationSubSystem().ClusterizeLights(VisblePointLightComponents, ViewMatrix);

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

	renderSystem.SwitchResourceState(*GBufferTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(*GBufferTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(HDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	renderSystem.SwitchResourceState(*ShadowMaskTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;

	void *ConstantBufferData;

	SAFE_DX(CPUDeferredLightingConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

	XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

	DeferredLightingConstantBuffer& ConstantBuffer = *((DeferredLightingConstantBuffer*)((BYTE*)ConstantBufferData));

	ConstantBuffer.InvViewProjMatrix = InvViewProjMatrix;
	ConstantBuffer.CameraWorldPosition = CameraLocation;

	WrittenRange.Begin = 0;
	WrittenRange.End = 256;

	CPUDeferredLightingConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	void *DynamicBufferData;

	SAFE_DX(CPULightClustersBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &DynamicBufferData));

	memcpy(DynamicBufferData, renderSystem.GetClusterizationSubSystem().GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

	WrittenRange.Begin = 0;
	WrittenRange.End = 32 * 18 * 24 * 2 * sizeof(uint32_t);

	CPULightClustersBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CPULightIndicesBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &DynamicBufferData));

	memcpy(DynamicBufferData, renderSystem.GetClusterizationSubSystem().GetLightIndicesData(), renderSystem.GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t));

	WrittenRange.Begin = 0;
	WrittenRange.End = renderSystem.GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t);

	CPULightIndicesBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CPUPointLightsBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &DynamicBufferData));

	memcpy(DynamicBufferData, PointLights.data(), PointLights.size() * sizeof(PointLight));

	WrittenRange.Begin = 0;
	WrittenRange.End = PointLights.size() * sizeof(PointLight);

	CPUPointLightsBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	renderSystem.SwitchResourceState(GPUDeferredLightingConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	renderSystem.SwitchResourceState(GPULightClustersBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	renderSystem.SwitchResourceState(GPULightIndicesBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	renderSystem.SwitchResourceState(GPUPointLightsBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->CopyBufferRegion(GPUDeferredLightingConstantBuffer.DXBuffer, 0, CPUDeferredLightingConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, 256);
	renderSystem.GetCommandList()->CopyBufferRegion(GPULightClustersBuffer.DXBuffer, 0, CPULightClustersBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * sizeof(LightCluster));
	renderSystem.GetCommandList()->CopyBufferRegion(GPULightIndicesBuffer.DXBuffer, 0, CPULightIndicesBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, renderSystem.GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t));
	renderSystem.GetCommandList()->CopyBufferRegion(GPUPointLightsBuffer.DXBuffer, 0, CPUPointLightsBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, PointLights.size() * sizeof(PointLight));

	renderSystem.SwitchResourceState(GPUDeferredLightingConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	renderSystem.SwitchResourceState(GPULightClustersBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(GPULightIndicesBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(GPUPointLightsBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = float(renderSystem.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(renderSystem.GetResolutionWidth());

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = renderSystem.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderSystem.GetResolutionWidth();
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	DeferredLightingCBTable[0] = DeferredLightingConstantBufferCBV;
	DeferredLightingCBTable.SetTableSize(1);
	DeferredLightingCBTable.UpdateDescriptorTable();

	DeferredLightingSRTable[0] = GBufferTexturesSRVs[0];
	DeferredLightingSRTable[1] = GBufferTexturesSRVs[1];
	DeferredLightingSRTable[2] = DepthBufferTextureSRV;
	DeferredLightingSRTable[3] = ShadowMaskTextureSRV;
	DeferredLightingSRTable[4] = LightClustersBufferSRV;
	DeferredLightingSRTable[5] = LightIndicesBufferSRV;
	DeferredLightingSRTable[6] = PointLightsBufferSRV;
	DeferredLightingSRTable.SetTableSize(7);
	DeferredLightingSRTable.UpdateDescriptorTable();

	renderSystem.GetCommandList()->DiscardResource(HDRSceneColorTexture.DXTexture, nullptr);

	renderSystem.GetCommandList()->SetPipelineState(DeferredLightingPipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_CONSTANT_BUFFERS, DeferredLightingCBTable);
	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, DeferredLightingSRTable);

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);
}