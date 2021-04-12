#pragma once

#include "../RenderStage.h"

class ShadowResolveStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "ShadowResolveStage"; }

	private:

};