#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class DeferredLightingPass : public RenderPass
{
	public:

		Texture* GetHDRSceneColorTexture() { return &HDRSceneColorTexture; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetHDRSceneColorTextureRTV() { return HDRSceneColorTextureRTV; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetHDRSceneColorTextureSRV() { return HDRSceneColorTextureSRV; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "DeferredLightingPass"; }

	private:

		Texture *GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesSRVs[2];

		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureSRV;

		Texture *ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureSRV;

		Texture HDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureRTV, HDRSceneColorTextureSRV;

		Buffer GPUDeferredLightingConstantBuffer, CPUDeferredLightingConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE DeferredLightingConstantBufferCBV;

		Buffer GPULightClustersBuffer, CPULightClustersBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightClustersBufferSRV;

		Buffer GPULightIndicesBuffer, CPULightIndicesBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightIndicesBufferSRV;

		Buffer GPUPointLightsBuffer, CPUPointLightsBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE PointLightsBufferSRV;

		COMRCPtr<ID3D12PipelineState> DeferredLightingPipelineState;

		DescriptorTable DeferredLightingCBTable, DeferredLightingSRTable;
};