#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ScenePass.h"

class GBufferOpaqueStage : public RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) override;
		virtual void Execute() override;

		virtual const char* GetName() override { return "GBufferOpaqueStage"; }

	private:

		ScenePass *GBufferOpaquePass;

};