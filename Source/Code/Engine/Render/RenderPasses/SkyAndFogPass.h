#pragma once

#include "../RenderPass.h"

class SkyAndFogPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};