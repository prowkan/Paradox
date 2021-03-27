#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class GBufferOpaquePass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		COMRCPtr<ID3D12Resource> GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesRTVs[2], GBufferTexturesSRVs[2];

		COMRCPtr<ID3D12Resource> DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureDSV, DepthBufferTextureSRV;

		COMRCPtr<ID3D12Resource> GPUConstantBuffer, CPUConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[20000];
};