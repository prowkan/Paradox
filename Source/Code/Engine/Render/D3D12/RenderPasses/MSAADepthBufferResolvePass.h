#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class MSAADepthBufferResolvePass : public RenderPass
{
	public:

		Texture* GetResolvedDepthBufferTexture() { return &ResolvedDepthBufferTexture; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetResolvedDepthBufferTextureSRV() { return ResolvedDepthBufferTextureSRV; }

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "MSAADepthBufferResolvePass"; }

	private:

		Texture *DepthBufferTexture;

		Texture ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;
};