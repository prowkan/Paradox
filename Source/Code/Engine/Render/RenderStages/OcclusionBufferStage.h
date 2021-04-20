#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/FullScreenPass.h"

class OcclusionBufferStage : public RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) override;
		virtual void Execute() override;

		virtual const char* GetName() override { return "OcclusionBufferStage"; }

	private:

		FullScreenPass *OcclusionBufferPass;
};