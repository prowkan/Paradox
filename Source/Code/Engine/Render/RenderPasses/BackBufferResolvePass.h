#pragma once

#include "../RenderPass.h"

#include <Containers/COMRCPtr.h>

class BackBufferResolvePass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

	private:

		COMRCPtr<ID3D12Resource> ToneMappedImageTexture;
};