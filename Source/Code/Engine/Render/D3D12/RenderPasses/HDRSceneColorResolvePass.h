#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class HDRSceneColorResolvePass : public RenderPass
{
	public:

		Texture* GetResolvedHDRSceneColorTexture() { return &ResolvedHDRSceneColorTexture; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetResolvedHDRSceneColorTextureSRV() { return ResolvedHDRSceneColorTextureSRV; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "HDRSceneColorResolvePass"; }

	private:

		Texture *HDRSceneColorTexture;

		Texture ResolvedHDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;
};