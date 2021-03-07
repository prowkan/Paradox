#pragma once

#include "../RenderPass.h"

class OcclusionBufferPass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};