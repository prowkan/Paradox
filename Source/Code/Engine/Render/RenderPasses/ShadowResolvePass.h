#pragma once

#include "../RenderPass.h"

class ShadowResolvePass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:

		COMRCPtr<ID3D12Resource> ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureRTV, ShadowMaskTextureSRV;

		COMRCPtr<ID3D12Resource> GPUShadowResolveConstantBuffer, CPUShadowResolveConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowResolveConstantBufferCBV;

		COMRCPtr<ID3D12PipelineState> ShadowResolvePipelineState;
};