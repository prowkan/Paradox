#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class MSAADepthBufferResolvePass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		COMRCPtr<ID3D12Resource> DepthBufferTexture;

		COMRCPtr<ID3D12Resource> ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;
};