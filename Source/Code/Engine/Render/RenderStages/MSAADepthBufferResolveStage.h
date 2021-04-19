#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ResolvePass.h"

class MSAADepthBufferResolveStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "MSAADepthBufferResolveStage"; }

	private:

		ResolvePass MSAADepthBufferResolvePass;
};