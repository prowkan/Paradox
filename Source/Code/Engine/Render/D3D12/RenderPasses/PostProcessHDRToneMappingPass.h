#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class PostProcessHDRToneMappingPass : public RenderPass
{
	public:

		Texture* GetToneMappedImageTexture() { return &ToneMappedImageTexture; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "PostProcessHDRToneMappingPass"; }

	private:

		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureSRV;

		Texture *OutputBloomTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE OutputBloomTextureSRV;

		Texture ToneMappedImageTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ToneMappedImageTextureRTV;

		COMRCPtr<ID3D12PipelineState> HDRToneMappingPipelineState;

		DescriptorTable HDRToneMappingPassSRTable;
};