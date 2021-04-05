#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class GBufferOpaquePass : public RenderPass
{
	public:

		Texture* GetGBufferTexture(UINT Index) { return &GBufferTextures[Index]; }
		Texture* GetDepthBufferTexture() { return &DepthBufferTexture; }

		D3D12_CPU_DESCRIPTOR_HANDLE GetGBufferTextureSRV(UINT Index) { return GBufferTexturesSRVs[Index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferTextureSRV() { return DepthBufferTextureSRV; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthBufferTextureDSV() { return DepthBufferTextureDSV; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "GBufferOpaquePass"; }

	private:

		Texture GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesRTVs[2], GBufferTexturesSRVs[2];

		Texture DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureDSV, DepthBufferTextureSRV;

		Buffer GPUConstantBuffer, CPUConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[20000];

		DescriptorTable ConstantBufferTables[20000], ShaderResourcesTables[20000];
};