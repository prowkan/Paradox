#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ResolvePass.h"

class MSAADepthBufferResolveStage : public RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) override;
		virtual void Execute() override;

		virtual const char* GetName() override { return "MSAADepthBufferResolveStage"; }

	private:

		ResolvePass *MSAADepthBufferResolvePass;
};