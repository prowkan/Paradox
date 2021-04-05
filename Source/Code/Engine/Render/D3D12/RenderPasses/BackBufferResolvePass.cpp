// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "BackBufferResolvePass.h"

#include "PostProcessHDRToneMappingPass.h"

#include "../RenderDeviceD3D12.h"

void BackBufferResolvePass::Init(RenderDeviceD3D12& renderDevice)
{
	ToneMappedImageTexture = ((PostProcessHDRToneMappingPass*)renderDevice.GetRenderPass("PostProcessHDRToneMappingPass"))->GetToneMappedImageTexture();
}

void BackBufferResolvePass::Execute(RenderDeviceD3D12& renderDevice)
{
	renderDevice.SwitchResourceState(*ToneMappedImageTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	renderDevice.SwitchResourceState(*renderDevice.GetCurrentBackBufferTexture(), 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST);

	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->ResolveSubresource(renderDevice.GetCurrentBackBufferTexture()->DXTexture, 0, ToneMappedImageTexture->DXTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
}