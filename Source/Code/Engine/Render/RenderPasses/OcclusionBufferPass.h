#pragma once

#include "../RenderPass.h"

class OcclusionBufferPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:

		COMRCPtr<ID3D12Resource> OcclusionBufferTexture, OcclusionBufferTextureReadback[2];
		D3D12_CPU_DESCRIPTOR_HANDLE OcclusionBufferTextureRTV;

		COMRCPtr<ID3D12PipelineState> OcclusionBufferPipelineState;
};