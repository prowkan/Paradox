#pragma once

#include "../RenderPass.h"

class ShadowMapPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};