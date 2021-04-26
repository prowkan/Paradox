#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class ShadowMapPass : public RenderPass
{
	public:

		Texture* GetCascadedShadowMapTexture(UINT Index) { return &CascadedShadowMapTextures[Index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCascadedShadowMapTextureSRV(UINT Index) { return CascadedShadowMapTexturesSRVs[Index]; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "ShadowMapPass"; }

	private:

		Texture CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesDSVs[4], CascadedShadowMapTexturesSRVs[4];

		Buffer GPUConstantBuffers[4], CPUConstantBuffers[4][2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[4][20000];

		DescriptorTable ConstantBufferTables[80000];
};