#pragma once

#include "../RenderPass.h"

class PostProcessLuminancePass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};