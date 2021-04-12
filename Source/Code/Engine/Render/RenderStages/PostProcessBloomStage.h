#pragma once

#include "../RenderStage.h"

class PostProcessBloomStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "PostProcessBloomStage"; }

	private:
};