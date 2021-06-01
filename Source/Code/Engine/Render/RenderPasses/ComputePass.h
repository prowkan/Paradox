#pragma once

#include "../RenderPass.h"

#include "../RenderGraph.h"

class ComputePass : public RenderPass
{
	public:

		void AddInput(RenderGraphResourceView* ResourceView) { ShaderResourceViews.Add(ResourceView); }
		void AddOutput(RenderGraphResourceView* ResourceView) { UnorderedAccessViews.Add(ResourceView); }

		virtual bool IsResourceUsedInRenderPass(RenderGraphResource* Resource) override
		{
			for (RenderGraphResourceView* ShaderResourceView : ShaderResourceViews)
			{
				if (ShaderResourceView->GetResource() == Resource) return true;
			}

			for (RenderGraphResourceView* UnorderedAccessView : UnorderedAccessViews)
			{
				if (UnorderedAccessView->GetResource() == Resource) return true;
			}

			return false;
		}

		virtual bool IsResourceReadInRenderPass(RenderGraphResource* Resource) override
		{
			for (RenderGraphResourceView* ShaderResourceView : ShaderResourceViews)
			{
				if (ShaderResourceView->GetResource() == Resource) return true;
			}

			return false;
		}

		virtual bool IsResourceWrittenInRenderPass(RenderGraphResource* Resource) override
		{
			for (RenderGraphResourceView* UnorderedAccessView : UnorderedAccessViews)
			{
				if (UnorderedAccessView->GetResource() == Resource) return true;
			}

			return false;
		}

		virtual ResourceUsageType GetResourceUsageType(RenderGraphResource* Resource) override
		{
			for (RenderGraphResourceView* ShaderResourceView : ShaderResourceViews)
			{
				if (ShaderResourceView->GetResource() == Resource) return ResourceUsageType::ShaderResource;
			}

			for (RenderGraphResourceView* UnorderedAccessView : UnorderedAccessViews)
			{
				if (UnorderedAccessView->GetResource() == Resource) return ResourceUsageType::UnorderedAccess;
			}
		}

	private:

		DynamicArray<RenderGraphResourceView*> ShaderResourceViews;
		DynamicArray<RenderGraphResourceView*> UnorderedAccessViews;
};