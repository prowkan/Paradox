#pragma once

#include "../RenderStage.h"

class HDRSceneColorResolveStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "HDRSceneColorResolveStage"; }

	private:
};