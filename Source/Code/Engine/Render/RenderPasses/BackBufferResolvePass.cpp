// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "BackBufferResolvePass.h"

#include "PostProcessHDRToneMappingPass.h"

#include "../RenderSystem.h"

void BackBufferResolvePass::Init(RenderSystem& renderSystem)
{
	ToneMappedImageTexture = ((PostProcessHDRToneMappingPass*)renderSystem.GetRenderPass("PostProcessHDRToneMappingPass"))->GetToneMappedImageTexture();
}

void BackBufferResolvePass::Execute(RenderSystem& renderSystem)
{
	renderSystem.SwitchResourceState(*ToneMappedImageTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	renderSystem.SwitchResourceState(*renderSystem.GetCurrentBackBufferTexture(), 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->ResolveSubresource(renderSystem.GetCurrentBackBufferTexture()->DXTexture, 0, ToneMappedImageTexture->DXTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
}