// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MSAADepthBufferResolvePass.h"

#include "GBufferOpaquePass.h"

#include "../RenderSystem.h"

#undef SAFE_DX
#define SAFE_DX(Func) Func

void MSAADepthBufferResolvePass::Init(RenderSystem& renderSystem)
{
	DepthBufferTexture = renderSystem.GetRenderPass<GBufferOpaquePass>()->GetDepthBufferTexture();

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	ResourceDesc.Height = renderSystem.GetResolutionHeight();
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
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
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	ResolvedDepthBufferTexture = renderSystem.CreateTexture(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	ResolvedDepthBufferTextureSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(ResolvedDepthBufferTexture.DXTexture, &SRVDesc, ResolvedDepthBufferTextureSRV);
}

void MSAADepthBufferResolvePass::Execute(RenderSystem& renderSystem)
{
	renderSystem.SwitchResourceState(*DepthBufferTexture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	renderSystem.SwitchResourceState(ResolvedDepthBufferTexture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST);

	renderSystem.ApplyPendingBarriers();

	COMRCPtr<ID3D12GraphicsCommandList1> CommandList1;

	renderSystem.GetCommandList()->QueryInterface<ID3D12GraphicsCommandList1>(&CommandList1);
	
	CommandList1->ResolveSubresourceRegion(ResolvedDepthBufferTexture.DXTexture, 0, 0, 0, DepthBufferTexture->DXTexture, 0, nullptr, DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT, D3D12_RESOLVE_MODE::D3D12_RESOLVE_MODE_MAX);
}