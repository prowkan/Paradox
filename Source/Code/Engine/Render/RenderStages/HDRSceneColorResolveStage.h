#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ResolvePass.h"

class HDRSceneColorResolveStage : public RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) override;
		virtual void Execute() override;

		virtual const char* GetName() override { return "HDRSceneColorResolveStage"; }

	private:

		ResolvePass *HDRSceneColorResolvePass;
};