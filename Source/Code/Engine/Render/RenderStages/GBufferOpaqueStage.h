#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ScenePass.h"

class GBufferOpaqueStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "GBufferOpaqueStage"; }

	private:

		ScenePass GBufferOpaquePass;

};