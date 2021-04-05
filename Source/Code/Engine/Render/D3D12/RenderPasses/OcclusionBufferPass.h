#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class OcclusionBufferPass : public RenderPass
{
	public:

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "OcclusionBufferPass"; }

	private:

		Texture *ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;

		Texture OcclusionBufferTexture;
		Buffer OcclusionBufferTextureReadback[2];
		D3D12_CPU_DESCRIPTOR_HANDLE OcclusionBufferTextureRTV;

		COMRCPtr<ID3D12PipelineState> OcclusionBufferPipelineState;

		DescriptorTable OcclusionBufferPassSRTable;
};