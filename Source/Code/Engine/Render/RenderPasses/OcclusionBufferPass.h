#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class OcclusionBufferPass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		COMRCPtr<ID3D12Resource> ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;

		COMRCPtr<ID3D12Resource> OcclusionBufferTexture, OcclusionBufferTextureReadback[2];
		D3D12_CPU_DESCRIPTOR_HANDLE OcclusionBufferTextureRTV;

		COMRCPtr<ID3D12PipelineState> OcclusionBufferPipelineState;
};