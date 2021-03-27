#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class ShadowMapPass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		COMRCPtr<ID3D12Resource> CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesDSVs[4], CascadedShadowMapTexturesSRVs[4];

		COMRCPtr<ID3D12Resource> GPUConstantBuffers[4], CPUConstantBuffers[4][2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[4][20000];
};