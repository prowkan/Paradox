#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class ShadowResolvePass : public RenderPass
{
	public:

		Texture* GetShadowMaskTexture() { return &ShadowMaskTexture; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetShadowMaskTextureSRV() { return ShadowMaskTextureSRV; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "ShadowResolvePass"; }

	private:

		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;

		Texture *CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesSRVs[4];

		Texture ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureRTV, ShadowMaskTextureSRV;

		Buffer GPUShadowResolveConstantBuffer, CPUShadowResolveConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowResolveConstantBufferCBV;

		COMRCPtr<ID3D12PipelineState> ShadowResolvePipelineState;

		DescriptorTable ShadowResolveCBTable, ShadowResolveSRTable;
};