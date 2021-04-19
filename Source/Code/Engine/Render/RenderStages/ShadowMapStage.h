#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ScenePass.h"

class ShadowMapStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "ShadowMapStage"; }

	private:

		ScenePass CascadedShadowMapPasses[4];
};