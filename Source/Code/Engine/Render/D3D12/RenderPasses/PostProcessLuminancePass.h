#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class PostProcessLuminancePass : public RenderPass
{
	public:

		D3D12_CPU_DESCRIPTOR_HANDLE GetSceneLuminanceTextureSRV() { return SceneLuminanceTexturesSRVs[0]; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "PostProcessLuminancePass"; }

	private:

		Texture *HDRSceneColorTexture;

		Texture *ResolvedHDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;

		Texture SceneLuminanceTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE SceneLuminanceTexturesUAVs[4], SceneLuminanceTexturesSRVs[4];

		Texture AverageLuminanceTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE AverageLuminanceTextureUAV, AverageLuminanceTextureSRV;

		COMRCPtr<ID3D12PipelineState> LuminanceCalcPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceSumPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceAvgPipelineState;

		DescriptorTable LuminancePassSRTables[5], LuminancePassUATables[5];
};