#pragma once

#include "../RenderPass.h"

class ResolvePass : public RenderPass
{
	friend class RenderGraph;

	public:

		void SetResolveInput(RenderGraphResource* Resource)
		{
			ResolveInput = Resource;
		}

		void SetResolveOutput(RenderGraphResource* Resource)
		{
			ResolveOutput = Resource;
		}

		virtual bool IsResolvePass() override
		{
			return true;
		}

	private:

		RenderGraphResource *ResolveInput;
		RenderGraphResource *ResolveOutput;
};