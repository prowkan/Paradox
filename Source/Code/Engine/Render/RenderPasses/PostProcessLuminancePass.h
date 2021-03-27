#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class PostProcessLuminancePass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		COMRCPtr<ID3D12Resource> HDRSceneColorTexture;

		COMRCPtr<ID3D12Resource> ResolvedHDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Resource> SceneLuminanceTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE SceneLuminanceTexturesUAVs[4], SceneLuminanceTexturesSRVs[4];

		COMRCPtr<ID3D12Resource> AverageLuminanceTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE AverageLuminanceTextureUAV, AverageLuminanceTextureSRV;

		COMRCPtr<ID3D12PipelineState> LuminanceCalcPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceSumPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceAvgPipelineState;
};