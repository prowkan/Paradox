#pragma once

#include "../RenderStage.h"

#include "../RenderPasses/ResolvePass.h"

class BackBufferResolveStage : public RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) override;
		virtual void Execute(RenderDevice* renderDevice) override;

		virtual const char* GetName() override { return "BackBufferResolveStage"; }

	private:

		ResolvePass BackBufferResolvePass;
};