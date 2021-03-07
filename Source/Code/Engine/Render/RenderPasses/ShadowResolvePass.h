#pragma once

#include "../RenderPass.h"

class ShadowResolvePass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};