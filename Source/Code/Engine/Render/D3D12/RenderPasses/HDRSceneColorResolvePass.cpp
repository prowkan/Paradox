// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "HDRSceneColorResolvePass.h"

#include "DeferredLightingPass.h"

#include "../RenderDeviceD3D12.h"

#undef SAFE_DX
#define SAFE_DX(Func) Func

void HDRSceneColorResolvePass::Init(RenderDeviceD3D12& renderDevice)
{
	HDRSceneColorTexture = ((DeferredLightingPass*)renderDevice.GetRenderPass("DeferredLightingPass"))->GetHDRSceneColorTexture();

	ResolvedHDRSceneColorTexture = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderDevice.GetResolutionWidth(), renderDevice.GetResolutionHeight(), DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	ResolvedHDRSceneColorTextureSRV = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateShaderResourceView(ResolvedHDRSceneColorTexture.DXTexture, &SRVDesc, ResolvedHDRSceneColorTextureSRV);
}

void HDRSceneColorResolvePass::Execute(RenderDeviceD3D12& renderDevice)
{
	renderDevice.SwitchResourceState(*HDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderDevice.SwitchResourceState(ResolvedHDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST);

	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->ResolveSubresource(ResolvedHDRSceneColorTexture.DXTexture, 0, HDRSceneColorTexture->DXTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT);
}