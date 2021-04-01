#pragma once

#include "../RenderPass.h"

#include "../RenderSystem.h"

#include <Containers/COMRCPtr.h>

class PostProcessBloomPass : public RenderPass
{
	public:

		Texture* GetOutputBloomTexture() { return &BloomTextures[2][0]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetOutputBloomTextureSRV() { return BloomTexturesSRVs[2][0]; }

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

		virtual const char* GetName() override { return "PostProcessBloomPass"; }

	private:

		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE SceneLuminanceTextureSRV;

		Texture BloomTextures[3][7];
		D3D12_CPU_DESCRIPTOR_HANDLE BloomTexturesRTVs[3][7], BloomTexturesSRVs[3][7];

		COMRCPtr<ID3D12PipelineState> BrightPassPipelineState;
		COMRCPtr<ID3D12PipelineState> DownSamplePipelineState;
		COMRCPtr<ID3D12PipelineState> HorizontalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> VerticalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> UpSampleWithAddBlendPipelineState;

		DescriptorTable BloomPassSRTables1[3], BloomPassSRTables2[6][3], BloomPassSRTables3[6];
};