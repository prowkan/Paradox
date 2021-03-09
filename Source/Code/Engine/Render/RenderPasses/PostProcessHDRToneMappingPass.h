#pragma once

#include "../RenderPass.h"

class PostProcessHDRToneMappingPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:

		COMRCPtr<ID3D12Resource> ToneMappedImageTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ToneMappedImageTextureRTV;

		COMRCPtr<ID3D12PipelineState> HDRToneMappingPipelineState;
};