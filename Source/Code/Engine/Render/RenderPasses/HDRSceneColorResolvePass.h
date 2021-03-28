#pragma once

#include "../RenderPass.h"

#include "../RenderSystem.h"

#include <Containers/COMRCPtr.h>

class HDRSceneColorResolvePass : public RenderPass
{
	public:

		Texture* GetResolvedHDRSceneColorTexture() { return &ResolvedHDRSceneColorTexture; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetResolvedHDRSceneColorTextureSRV() { return ResolvedHDRSceneColorTextureSRV; }

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		Texture *HDRSceneColorTexture;

		Texture ResolvedHDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;
};