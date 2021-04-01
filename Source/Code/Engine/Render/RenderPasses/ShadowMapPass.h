#pragma once

#include "../RenderPass.h"

#include "../RenderSystem.h"

#include <Containers/COMRCPtr.h>

class ShadowMapPass : public RenderPass
{
	public:

		Texture* GetCascadedShadowMapTexture(UINT Index) { return &CascadedShadowMapTextures[Index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCascadedShadowMapTextureSRV(UINT Index) { return CascadedShadowMapTexturesSRVs[Index]; }

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

		virtual const char* GetName() override { return "ShadowMapPass"; }

	private:

		Texture CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesDSVs[4], CascadedShadowMapTexturesSRVs[4];

		Buffer GPUConstantBuffers[4], CPUConstantBuffers[4][2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[4][20000];

		DescriptorTable ConstantBufferTables[80000];
};