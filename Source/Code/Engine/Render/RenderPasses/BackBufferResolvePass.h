#pragma once

#include "../RenderPass.h"

#include "../RenderSystem.h"

#include <Containers/COMRCPtr.h>

class BackBufferResolvePass : public RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) override;
		virtual void Execute(RenderSystem& renderSystem) override;

		virtual const char* GetName() override { return "BackBufferResolvePass"; }

	private:

		Texture *ToneMappedImageTexture;
};