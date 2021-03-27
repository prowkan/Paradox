// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ShadowMapPass.h"

#include "../RenderSystem.h"

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

struct ShadowMapPassConstantBuffer
{
	XMMATRIX WVPMatrix;
};

void ShadowMapPass::Init(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	ResourceDesc.Height = 2048;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 2048;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[2])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[3])));

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

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers[1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers[2])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers[3])));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[0][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[1][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[2][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[3][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[0][1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[1][1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[2][1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers[3][1])));

	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 20000; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = GPUConstantBuffers[j]->GetGPUVirtualAddress() + i * 256;
			CBVDesc.SizeInBytes = 256;

			ConstantBufferCBVs[j][i].ptr = ConstantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + ConstantBufferDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ConstantBufferDescriptorsCount++;

			Device->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs[j][i]);
		}
	}
}

void ShadowMapPass::Execute(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_BARRIER ResourceBarriers[5];

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

	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	Camera& camera = gameFramework.GetCamera();

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

	for (int i = 0; i < 4; i++)
	{
		SIZE_T ConstantBufferOffset = 0;

		vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
		vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i], false);
		size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUConstantBuffers[i][CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

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

		CPUConstantBuffers[i][CurrentFrameIndex]->Unmap(0, &WrittenRange);

		if (i == 0)
		{
			ResourceBarriers[4].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[4].Transition.pResource = GPUConstantBuffers[i];
			ResourceBarriers[4].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
			ResourceBarriers[4].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			ResourceBarriers[4].Transition.Subresource = 0;
			ResourceBarriers[4].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			renderSystem.GetCommandList()->ResourceBarrier(5, ResourceBarriers);
		}
		else
		{
			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = GPUConstantBuffers[i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			renderSystem.GetCommandList()->ResourceBarrier(1, ResourceBarriers);
		}

		renderSystem.GetCommandList()->CopyBufferRegion(GPUConstantBuffers[i], 0, CPUConstantBuffers[i][CurrentFrameIndex], 0, ConstantBufferOffset);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUConstantBuffers[i];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		renderSystem.GetCommandList()->ResourceBarrier(1, ResourceBarriers);

		renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		renderSystem.GetCommandList()->OMSetRenderTargets(0, nullptr, TRUE, &CascadedShadowMapTexturesDSVs[i]);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = 2048.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = 2048.0f;

		renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = 2048;
		ScissorRect.left = 0;
		ScissorRect.right = 2048;
		ScissorRect.top = 0;

		renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

		renderSystem.GetCommandList()->ClearDepthStencilView(CascadedShadowMapTexturesDSVs[i], D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

			RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
			RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
			RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
			RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

			UINT DestRangeSize = 1;
			UINT SourceRangeSizes[1] = { 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[1] = { ConstantBufferCBVs[i][k] };

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

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

			renderSystem.GetCommandList()->SetPipelineState(renderMaterial->ShadowMapPassPipelineState);

			renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(0, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			renderSystem.GetCommandList()->DrawIndexedInstanced(8 * 8 * 6 * 6, 1, 0, 0, 0);
		}
	}
}