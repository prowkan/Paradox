#pragma once

#include "../RenderPass.h"

#include "../RenderSystem.h"

#include <Containers/COMRCPtr.h>

class MSAADepthBufferResolvePass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

		Texture* GetResolvedDepthBufferTexture() { return &ResolvedDepthBufferTexture; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetResolvedDepthBufferTextureSRV() { return ResolvedDepthBufferTextureSRV; }

	private:

		Texture *DepthBufferTexture;

		Texture ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;
};