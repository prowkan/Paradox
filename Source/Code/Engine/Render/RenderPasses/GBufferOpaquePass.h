#pragma once

#include "../RenderPass.h"

class GBufferOpaquePass : public RenderPass
{
	public:

		virtual void Init() override;
		virtual void Execute() override;

	private:
};