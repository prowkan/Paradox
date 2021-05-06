#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ResolvePass.h"

class BackBufferResolveStage : public RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) override;
		virtual void Execute() override;

		virtual const char* GetName() override { return "BackBufferResolveStage"; }

	private:

		ResolvePass *BackBufferResolvePass;
};