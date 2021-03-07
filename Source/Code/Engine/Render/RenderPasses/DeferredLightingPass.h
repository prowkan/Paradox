#pragma once

#include "../RenderPass.h"

class DeferredLightingPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};