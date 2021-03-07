#pragma once

#include "../RenderPass.h"

class PostProcessHDRToneMappingPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};