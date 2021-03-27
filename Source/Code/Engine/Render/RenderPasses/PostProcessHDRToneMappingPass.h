#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class PostProcessHDRToneMappingPass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Resource> OutputBloomTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE OutputBloomTextureSRV;

		COMRCPtr<ID3D12Resource> ToneMappedImageTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ToneMappedImageTextureRTV;

		COMRCPtr<ID3D12PipelineState> HDRToneMappingPipelineState;
};