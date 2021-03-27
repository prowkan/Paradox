#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class ShadowResolvePass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;

		COMRCPtr<ID3D12Resource> CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesDSVs[4], CascadedShadowMapTexturesSRVs[4];

		COMRCPtr<ID3D12Resource> ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureRTV, ShadowMaskTextureSRV;

		COMRCPtr<ID3D12Resource> GPUShadowResolveConstantBuffer, CPUShadowResolveConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowResolveConstantBufferCBV;

		COMRCPtr<ID3D12PipelineState> ShadowResolvePipelineState;
};