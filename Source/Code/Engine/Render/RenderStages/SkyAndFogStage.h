#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/FullScreenPass.h"
#include "../RenderPass.h"

class SkyAndFogStage : public RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) override;
		virtual void Execute() override;

		virtual const char* GetName() override { return "SkyAndFogStage"; }

	private:

		FullScreenPass *FogPass;
		RenderPass *SkyPass;
};