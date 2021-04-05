// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ShadowMapPass.h"

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

struct ShadowMapPassConstantBuffer
{
	XMMATRIX WVPMatrix;
};

void ShadowMapPass::Init(RenderDeviceD3D12& renderDevice)
{
	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

	CascadedShadowMapTextures[0] = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(2048, 2048, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue);
	CascadedShadowMapTextures[1] = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(2048, 2048, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue);
	CascadedShadowMapTextures[2] = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(2048, 2048, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue);
	CascadedShadowMapTextures[3] = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(2048, 2048, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE, &ClearValue);
	
	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	DSVDesc.Texture2D.MipSlice = 0;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

	CascadedShadowMapTexturesDSVs[0] = renderDevice.GetDSDescriptorHeap().AllocateDescriptor();
	CascadedShadowMapTexturesDSVs[1] = renderDevice.GetDSDescriptorHeap().AllocateDescriptor();
	CascadedShadowMapTexturesDSVs[2] = renderDevice.GetDSDescriptorHeap().AllocateDescriptor();
	CascadedShadowMapTexturesDSVs[3] = renderDevice.GetDSDescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateDepthStencilView(CascadedShadowMapTextures[0].DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[0]);
	renderDevice.GetDevice()->CreateDepthStencilView(CascadedShadowMapTextures[1].DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[1]);
	renderDevice.GetDevice()->CreateDepthStencilView(CascadedShadowMapTextures[2].DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[2]);
	renderDevice.GetDevice()->CreateDepthStencilView(CascadedShadowMapTextures[3].DXTexture, &DSVDesc, CascadedShadowMapTexturesDSVs[3]);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	CascadedShadowMapTexturesSRVs[0] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();
	CascadedShadowMapTexturesSRVs[1] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();
	CascadedShadowMapTexturesSRVs[2] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();
	CascadedShadowMapTexturesSRVs[3] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateShaderResourceView(CascadedShadowMapTextures[0].DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[0]);
	renderDevice.GetDevice()->CreateShaderResourceView(CascadedShadowMapTextures[1].DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[1]);
	renderDevice.GetDevice()->CreateShaderResourceView(CascadedShadowMapTextures[2].DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[2]);
	renderDevice.GetDevice()->CreateShaderResourceView(CascadedShadowMapTextures[3].DXTexture, &SRVDesc, CascadedShadowMapTexturesSRVs[3]);

	GPUConstantBuffers[0] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);
	GPUConstantBuffers[1] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);
	GPUConstantBuffers[2] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);
	GPUConstantBuffers[3] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);

	CPUConstantBuffers[0][0] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[1][0] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[2][0] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[3][0] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[0][1] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[1][1] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[2][1] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUConstantBuffers[3][1] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256 * 20000), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 20000; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = GPUConstantBuffers[j].DXBuffer->GetGPUVirtualAddress() + i * 256;
			CBVDesc.SizeInBytes = 256;

			ConstantBufferCBVs[j][i] = renderDevice.GetConstantBufferDescriptorHeap().AllocateDescriptor();

			renderDevice.GetDevice()->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs[j][i]);
		}
	}

	for (UINT i = 0; i < 80000; i++)
	{
		ConstantBufferTables[i] = renderDevice.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderDevice.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderDeviceD3D12::VERTEX_SHADER_CONSTANT_BUFFERS]);
	}
}

void ShadowMapPass::Execute(RenderDeviceD3D12& renderDevice)
{
	renderDevice.SwitchResourceState(CascadedShadowMapTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);
	renderDevice.SwitchResourceState(CascadedShadowMapTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);
	renderDevice.SwitchResourceState(CascadedShadowMapTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);
	renderDevice.SwitchResourceState(CascadedShadowMapTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);

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
		vector<StaticMeshComponent*> VisbleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i], false);
		size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUConstantBuffers[i][renderDevice.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

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

		CPUConstantBuffers[i][renderDevice.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

		renderDevice.SwitchResourceState(GPUConstantBuffers[i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		renderDevice.ApplyPendingBarriers();

		renderDevice.GetCommandList()->CopyBufferRegion(GPUConstantBuffers[i].DXBuffer, 0, CPUConstantBuffers[i][renderDevice.GetCurrentFrameIndex()].DXBuffer, 0, ConstantBufferOffset);

		renderDevice.SwitchResourceState(GPUConstantBuffers[i], D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		renderDevice.ApplyPendingBarriers();

		renderDevice.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		renderDevice.GetCommandList()->OMSetRenderTargets(0, nullptr, TRUE, &CascadedShadowMapTexturesDSVs[i]);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = 2048.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = 2048.0f;

		renderDevice.GetCommandList()->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = 2048;
		ScissorRect.left = 0;
		ScissorRect.right = 2048;
		ScissorRect.top = 0;

		renderDevice.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

		renderDevice.GetCommandList()->ClearDepthStencilView(CascadedShadowMapTexturesDSVs[i], D3D12_CLEAR_FLAGS::D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		for (size_t k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

			RenderMeshD3D12 *renderMesh = (RenderMeshD3D12*)staticMeshComponent->GetStaticMesh()->GetRenderMesh();
			RenderMaterialD3D12 *renderMaterial = (RenderMaterialD3D12*)staticMeshComponent->GetMaterial()->GetRenderMaterial();
			RenderTextureD3D12 *renderTexture0 = (RenderTextureD3D12*)staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
			RenderTextureD3D12 *renderTexture1 = (RenderTextureD3D12*)staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

			ConstantBufferTables[i * 20000 + k][0] = ConstantBufferCBVs[i][k];
			ConstantBufferTables[i * 20000 + k].SetTableSize(1);
			ConstantBufferTables[i * 20000 + k].UpdateDescriptorTable();

			UINT DestRangeSize = 1;
			UINT SourceRangeSizes[1] = { 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[1] = { ConstantBufferCBVs[i][k] };

			D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
			VertexBufferView.BufferLocation = renderMesh->VertexBufferAddresses[0];
			VertexBufferView.SizeInBytes = sizeof(XMFLOAT3) * 9 * 9 * 6;
			VertexBufferView.StrideInBytes = sizeof(XMFLOAT3);

			D3D12_INDEX_BUFFER_VIEW IndexBufferView;
			IndexBufferView.BufferLocation = renderMesh->IndexBufferAddress;
			IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
			IndexBufferView.SizeInBytes = sizeof(WORD) * 8 * 8 * 6 * 6;

			renderDevice.GetCommandList()->IASetVertexBuffers(0, 1, &VertexBufferView);
			renderDevice.GetCommandList()->IASetIndexBuffer(&IndexBufferView);

			renderDevice.GetCommandList()->SetPipelineState(renderMaterial->ShadowMapPassPipelineState);

			renderDevice.GetCommandList()->SetGraphicsRootDescriptorTable(RenderDeviceD3D12::VERTEX_SHADER_CONSTANT_BUFFERS, ConstantBufferTables[i * 20000 + k]);

			renderDevice.GetCommandList()->DrawIndexedInstanced(8 * 8 * 6 * 6, 1, 0, 0, 0);
		}
	}
}