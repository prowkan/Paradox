// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GBufferOpaquePass.h"

#include "../RenderSystem.h"

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

void GBufferOpaquePass::Init(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Height = renderSystem.GetResolutionHeight();
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = renderSystem.GetResolutionWidth();

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	GBufferTextures[0] = renderSystem.CreateTexture(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);

	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;

	GBufferTextures[1] = renderSystem.CreateTexture(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);
	
	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	GBufferTexturesRTVs[0] = renderSystem.GetRTDescriptorHeap().AllocateDescriptor();
	GBufferTexturesRTVs[1] = renderSystem.GetRTDescriptorHeap().AllocateDescriptor();

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderSystem.GetDevice()->CreateRenderTargetView(GBufferTextures[0].DXTexture, &RTVDesc, GBufferTexturesRTVs[0]);

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	renderSystem.GetDevice()->CreateRenderTargetView(GBufferTextures[1].DXTexture, &RTVDesc, GBufferTexturesRTVs[1]);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	GBufferTexturesSRVs[0] = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();
	GBufferTexturesSRVs[1] = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderSystem.GetDevice()->CreateShaderResourceView(GBufferTextures[0].DXTexture, &SRVDesc, GBufferTexturesSRVs[0]);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	renderSystem.GetDevice()->CreateShaderResourceView(GBufferTextures[1].DXTexture, &SRVDesc, GBufferTexturesSRVs[1]);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	ResourceDesc.Height = renderSystem.GetResolutionHeight();
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = renderSystem.GetResolutionWidth();

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	ClearValue.DepthStencil.Depth = 0.0f;
	ClearValue.DepthStencil.Stencil = 0;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	DepthBufferTexture = renderSystem.CreateTexture(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue);

	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DMS;

	DepthBufferTextureDSV = renderSystem.GetDSDescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateDepthStencilView(DepthBufferTexture.DXTexture, &DSVDesc, DepthBufferTextureDSV);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	DepthBufferTextureSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(DepthBufferTexture.DXTexture, &SRVDesc, DepthBufferTextureSRV);

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
	ResourceDesc.Width = 256 * 20000;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	GPUConstantBuffer = renderSystem.CreateBuffer(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	CPUConstantBuffers[0] = renderSystem.CreateBuffer(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[1] = renderSystem.CreateBuffer(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	
	for (int i = 0; i < 20000; i++)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
		CBVDesc.BufferLocation = GPUConstantBuffer.DXBuffer->GetGPUVirtualAddress() + i * 256;
		CBVDesc.SizeInBytes = 256;

		ConstantBufferCBVs[i] = renderSystem.GetConstantBufferDescriptorHeap().AllocateDescriptor();

		renderSystem.GetDevice()->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs[i]);
	}

	for (UINT i = 0; i < 20000; i++)
	{
		ConstantBufferTables[i] = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::VERTEX_SHADER_CONSTANT_BUFFERS]);
		ConstantBufferTables[i].SetTableSize(1);
		ConstantBufferTables[i][0] = ConstantBufferCBVs[i];
		ConstantBufferTables[i].UpdateDescriptorTable(renderSystem.GetDevice());
	}
}

void GBufferOpaquePass::Execute(RenderSystem& renderSystem)
{
	if (First)
	{
		for (UINT i = 0; i < 4000; i++)
		{
			ShaderResourcesTables[i] = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);
			ShaderResourcesTables[i].SetTableSize(2);

			char MaterialResourceName[255];

			sprintf(MaterialResourceName, "Standart_%d", i);

			MaterialResource *Material = Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName);
			RenderTexture *renderTexture0 = Material->GetTexture(0)->GetRenderTexture();
			RenderTexture *renderTexture1 = Material->GetTexture(1)->GetRenderTexture();

			ShaderResourcesTables[i][0] = renderTexture0->TextureSRV;
			ShaderResourcesTables[i][1] = renderTexture1->TextureSRV;

			ShaderResourcesTables[i].UpdateDescriptorTable(renderSystem.GetDevice());
		}

		First = false;
	}

	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	RenderScene& renderScene = gameFramework.GetWorld().GetRenderScene();

	Camera& camera = gameFramework.GetCamera();

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	vector<StaticMeshComponent*> AllStaticMeshComponents = renderScene.GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = renderSystem.GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	renderSystem.SwitchResourceState(GBufferTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	renderSystem.SwitchResourceState(GBufferTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;

	void *ConstantBufferData;
	SIZE_T ConstantBufferOffset = 0;

	SAFE_DX(CPUConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

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

	CPUConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	renderSystem.SwitchResourceState(GPUConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->CopyBufferRegion(GPUConstantBuffer.DXBuffer, 0, CPUConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, ConstantBufferOffset);

	renderSystem.SwitchResourceState(GPUConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	renderSystem.GetCommandList()->OMSetRenderTargets(2, GBufferTexturesRTVs, TRUE, &DepthBufferTextureDSV);

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

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	renderSystem.GetCommandList()->ClearRenderTargetView(GBufferTexturesRTVs[0], ClearColor, 0, nullptr);
	renderSystem.GetCommandList()->ClearRenderTargetView(GBufferTexturesRTVs[1], ClearColor, 0, nullptr);
	renderSystem.GetCommandList()->ClearDepthStencilView(DepthBufferTextureDSV, D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SAMPLERS, renderSystem.GetTextureSamplerTable());

	for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

		RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
		RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		VertexBufferView.BufferLocation = renderMesh->VertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * 9 * 9 * 6;
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		IndexBufferView.BufferLocation = renderMesh->IndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * 8 * 8 * 6 * 6;

		renderSystem.GetCommandList()->IASetVertexBuffers(0, 1, &VertexBufferView);
		renderSystem.GetCommandList()->IASetIndexBuffer(&IndexBufferView);

		renderSystem.GetCommandList()->SetPipelineState(renderMaterial->GBufferOpaquePassPipelineState);

		renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::VERTEX_SHADER_CONSTANT_BUFFERS, ConstantBufferTables[k]);
		renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, ShaderResourcesTables[k % 4000]);

		renderSystem.GetCommandList()->DrawIndexedInstanced(8 * 8 * 6 * 6, 1, 0, 0, 0);
	}
}