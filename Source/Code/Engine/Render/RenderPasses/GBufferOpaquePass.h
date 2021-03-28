#pragma once

#include "../RenderPass.h"

#include "../RenderSystem.h"

#include <Containers/COMRCPtr.h>

class GBufferOpaquePass : public RenderPass
{
	public:

		Texture* GetGBufferTexture(UINT Index) { return &GBufferTextures[Index]; }
		Texture* GetDepthBufferTexture() { return &DepthBufferTexture; }

		D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferTextureSRV(UINT Index) { return GBufferTexturesSRVs[Index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferTextureSRV() { return DepthBufferTextureSRV; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferTextureDSV() { return DepthBufferTextureDSV; }

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		Texture GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesRTVs[2], GBufferTexturesSRVs[2];

		Texture DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureDSV, DepthBufferTextureSRV;

		Buffer GPUConstantBuffer, CPUConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[20000];

		DescriptorTable ConstantBufferTables[20000], ShaderResourcesTables[4000];

		bool First = true;
};