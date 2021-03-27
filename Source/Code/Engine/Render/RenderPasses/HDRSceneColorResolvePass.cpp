// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "HDRSceneColorResolvePass.h"

#include "../RenderSystem.h"

void HDRSceneColorResolvePass::Init(RenderSystem& renderSystem)
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

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(ResolvedHDRSceneColorTexture)));

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

void HDRSceneColorResolvePass::Execute(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_BARRIER ResourceBarriers[2];

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

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	renderSystem.GetCommandList()->ResolveSubresource(ResolvedHDRSceneColorTexture, 0, HDRSceneColorTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT);
}