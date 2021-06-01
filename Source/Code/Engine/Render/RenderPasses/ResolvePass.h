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

		virtual bool IsResourceUsedInRenderPass(RenderGraphResource* Resource) override
		{
			return (Resource == ResolveInput || Resource == ResolveOutput);
		}

		virtual bool IsResourceReadInRenderPass(RenderGraphResource* Resource) override
		{
			return (Resource == ResolveInput);
		}

		virtual bool IsResourceWrittenInRenderPass(RenderGraphResource* Resource) override
		{
			return (Resource == ResolveOutput);
		}

		virtual ResourceUsageType GetResourceUsageType(RenderGraphResource* Resource) override
		{
			if (Resource == ResolveInput) return ResourceUsageType::ResolveInput;
			if (Resource == ResolveOutput) return ResourceUsageType::ResolveOutput;
		}

	private:

		RenderGraphResource *ResolveInput;
		RenderGraphResource *ResolveOutput;
};