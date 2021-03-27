#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class DeferredLightingPass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		COMRCPtr<ID3D12Resource> GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesSRVs[2];

		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureSRV;

		COMRCPtr<ID3D12Resource> ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureSRV;

		COMRCPtr<ID3D12Resource> HDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureRTV, HDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Resource> GPUDeferredLightingConstantBuffer, CPUDeferredLightingConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE DeferredLightingConstantBufferCBV;

		COMRCPtr<ID3D12Resource> GPULightClustersBuffer, CPULightClustersBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightClustersBufferSRV;

		COMRCPtr<ID3D12Resource> GPULightIndicesBuffer, CPULightIndicesBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightIndicesBufferSRV;

		COMRCPtr<ID3D12Resource> GPUPointLightsBuffer, CPUPointLightsBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE PointLightsBufferSRV;

		COMRCPtr<ID3D12PipelineState> DeferredLightingPipelineState;
};