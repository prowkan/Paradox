#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ComputePass.h"

class PostProcessLuminanceStage : public RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) override;
		virtual void Execute() override;

		virtual const char* GetName() override { return "PostProcessLuminanceStage"; }

	private:

		ComputePass **SceneLuminancePasses;
};