#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/FullScreenPass.h"

class PostProcessBloomStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "PostProcessBloomStage"; }

	private:

		FullScreenPass *BloomPasses;
};