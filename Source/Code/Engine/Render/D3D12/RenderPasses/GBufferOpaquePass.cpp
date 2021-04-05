// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GBufferOpaquePass.h"

#include "../RenderDeviceD3D12.h"

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

struct GBufferOpaquePassConstantBuffer
{
	XMMATRIX WVPMatrix;
	XMMATRIX WorldMatrix;
	XMFLOAT3X4 VectorTransformMatrix;
};

void GBufferOpaquePass::Init(RenderDeviceD3D12& renderDevice)
{
	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GBufferTextures[0] = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderDevice.GetResolutionWidth(), renderDevice.GetResolutionHeight(), DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 1, 8), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	GBufferTextures[1] = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderDevice.GetResolutionWidth(), renderDevice.GetResolutionHeight(), DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 1, 8), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);
	
	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	GBufferTexturesRTVs[0] = renderDevice.GetRTDescriptorHeap().AllocateDescriptor();
	GBufferTexturesRTVs[1] = renderDevice.GetRTDescriptorHeap().AllocateDescriptor();

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderDevice.GetDevice()->CreateRenderTargetView(GBufferTextures[0].DXTexture, &RTVDesc, GBufferTexturesRTVs[0]);

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	renderDevice.GetDevice()->CreateRenderTargetView(GBufferTextures[1].DXTexture, &RTVDesc, GBufferTexturesRTVs[1]);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	GBufferTexturesSRVs[0] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();
	GBufferTexturesSRVs[1] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderDevice.GetDevice()->CreateShaderResourceView(GBufferTextures[0].DXTexture, &SRVDesc, GBufferTexturesSRVs[0]);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	renderDevice.GetDevice()->CreateShaderResourceView(GBufferTextures[1].DXTexture, &SRVDesc, GBufferTexturesSRVs[1]);

	ClearValue.DepthStencil.Depth = 0.0f;
	ClearValue.DepthStencil.Stencil = 0;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	DepthBufferTexture = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderDevice.GetResolutionWidth(), renderDevice.GetResolutionHeight(), DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 1, 8), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue);

	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DMS;

	DepthBufferTextureDSV = renderDevice.GetDSDescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateDepthStencilView(DepthBufferTexture.DXTexture, &DSVDesc, DepthBufferTextureDSV);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	DepthBufferTextureSRV = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateShaderResourceView(DepthBufferTexture.DXTexture, &SRVDesc, DepthBufferTextureSRV);

	GPUConstantBuffer = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);

	CPUConstantBuffers[0] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[1] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	
	for (int i = 0; i < 20000; i++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUConstantBuffer.DXBuffer->GetGPUVirtualAddress() + i * 256;
		CBVDesc.SizeInBytes = 256;

		ConstantBufferCBVs[i] = renderDevice.GetConstantBufferDescriptorHeap().AllocateDescriptor();

		renderDevice.GetDevice()->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs[i]);
	}

	for (UINT i = 0; i < 20000; i++)
	{
		ConstantBufferTables[i] = renderDevice.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderDevice.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderDeviceD3D12::VERTEX_SHADER_CONSTANT_BUFFERS]);
		ShaderResourcesTables[i] = renderDevice.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderDevice.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderDeviceD3D12::PIXEL_SHADER_SHADER_RESOURCES]);

	}
}

void GBufferOpaquePass::Execute(RenderDeviceD3D12& renderDevice)
{
	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	RenderScene& renderScene = gameFramework.GetWorld().GetRenderScene();

	Camera& camera = gameFramework.GetCamera();

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	vector<StaticMeshComponent*> AllStaticMeshComponents = renderScene.GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	renderDevice.SwitchResourceState(GBufferTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	renderDevice.SwitchResourceState(GBufferTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;

	void *ConstantBufferData;
	SIZE_T ConstantBufferOffset = 0;

	SAFE_DX(CPUConstantBuffers[renderDevice.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

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

	CPUConstantBuffers[renderDevice.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	renderDevice.SwitchResourceState(GPUConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->CopyBufferRegion(GPUConstantBuffer.DXBuffer, 0, CPUConstantBuffers[renderDevice.GetCurrentFrameIndex()].DXBuffer, 0, ConstantBufferOffset);

	renderDevice.SwitchResourceState(GPUConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	renderDevice.GetCommandList()->OMSetRenderTargets(2, GBufferTexturesRTVs, TRUE, &DepthBufferTextureDSV);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = float(renderDevice.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(renderDevice.GetResolutionWidth());

	renderDevice.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = renderDevice.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderDevice.GetResolutionWidth();
	ScissorRect.top = 0;

	renderDevice.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	renderDevice.GetCommandList()->ClearRenderTargetView(GBufferTexturesRTVs[0], ClearColor, 0, nullptr);
	renderDevice.GetCommandList()->ClearRenderTargetView(GBufferTexturesRTVs[1], ClearColor, 0, nullptr);
	renderDevice.GetCommandList()->ClearDepthStencilView(DepthBufferTextureDSV, D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);

	renderDevice.GetCommandList()->SetGraphicsRootDescriptorTable(RenderDeviceD3D12::PIXEL_SHADER_SAMPLERS, renderDevice.GetTextureSamplerTable());

	for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

		RenderMeshD3D12 *renderMesh = (RenderMeshD3D12*)staticMeshComponent->GetStaticMesh()->GetRenderMesh();
		RenderMaterialD3D12 *renderMaterial = (RenderMaterialD3D12*)staticMeshComponent->GetMaterial()->GetRenderMaterial();
		MaterialResource *material = staticMeshComponent->GetMaterial();
		RenderTextureD3D12 *renderTexture0 = (RenderTextureD3D12*)material->GetTexture(0)->GetRenderTexture();
		RenderTextureD3D12 *renderTexture1 = (RenderTextureD3D12*)material->GetTexture(1)->GetRenderTexture();

		ConstantBufferTables[k][0] = ConstantBufferCBVs[k];
		ConstantBufferTables[k].SetTableSize(1);
		ConstantBufferTables[k].UpdateDescriptorTable();

		ShaderResourcesTables[k][0] = renderTexture0->TextureSRV;
		ShaderResourcesTables[k][1] = renderTexture1->TextureSRV;
		ShaderResourcesTables[k].SetTableSize(2);
		ShaderResourcesTables[k].UpdateDescriptorTable();

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

		renderDevice.GetCommandList()->IASetVertexBuffers(0, 3, VertexBufferViews);
		renderDevice.GetCommandList()->IASetIndexBuffer(&IndexBufferView);

		renderDevice.GetCommandList()->SetPipelineState(renderMaterial->GBufferOpaquePassPipelineState);

		renderDevice.GetCommandList()->SetGraphicsRootDescriptorTable(RenderDeviceD3D12::VERTEX_SHADER_CONSTANT_BUFFERS, ConstantBufferTables[k]);
		renderDevice.GetCommandList()->SetGraphicsRootDescriptorTable(RenderDeviceD3D12::PIXEL_SHADER_SHADER_RESOURCES, ShaderResourcesTables[k]);

		renderDevice.GetCommandList()->DrawIndexedInstanced(8 * 8 * 6 * 6, 1, 0, 0, 0);
	}
}