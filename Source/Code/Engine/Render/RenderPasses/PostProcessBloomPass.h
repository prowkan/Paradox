#pragma once

#include "../RenderPass.h"

class PostProcessBloomPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};