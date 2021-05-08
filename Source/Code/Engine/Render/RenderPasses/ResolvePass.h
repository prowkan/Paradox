#pragma once

#include "../RenderPass.h"

class ResolvePass : public RenderPass
{
	public:

		void SetResolveInput(RenderGraphResource* Resource)
		{
			ResolveInput = Resource;
		}

		void SetResolveOutput(RenderGraphResource* Resource)
		{
			ResolveOutput = Resource;
		}

	private:

		RenderGraphResource *ResolveInput;
		RenderGraphResource *ResolveOutput;
};