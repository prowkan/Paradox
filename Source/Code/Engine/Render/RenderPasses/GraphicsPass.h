#pragma once

#include "../RenderPass.h"

#include "../RenderGraph.h"

class GraphicsPass : public RenderPass
{
	public:

		void SetRenderTarget(RenderGraphResourceView* ResourceView, UINT RenderTargetIndex) { RenderTargetViews[RenderTargetIndex] = ResourceView; }
		void SetDepthStencil(RenderGraphResourceView* ResourceView) { DepthStencilView = ResourceView; }

		void AddInput(RenderGraphResourceView* ResourceView) { ShaderResourceViews.Add(ResourceView); }

		GraphicsPass()
		{
			for (UINT i = 0; i < 8; i++)
			{
				RenderTargetViews[i] = nullptr;
			}

			DepthStencilView = nullptr;
		}

		virtual bool IsResourceUsedInRenderPass(RenderGraphResource* Resource) override
		{
			if (DepthStencilView && DepthStencilView->GetResource() == Resource) return true;

			for (UINT i = 0; i < 8; i++)
			{
				if (RenderTargetViews[i] && RenderTargetViews[i]->GetResource() == Resource) return true;
			}

			for (RenderGraphResourceView* ShaderResourceView : ShaderResourceViews)
			{
				if (ShaderResourceView->GetResource() == Resource) return true;
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
			if (DepthStencilView && DepthStencilView->GetResource() == Resource) return true;

			for (UINT i = 0; i < 8; i++)
			{
				if (RenderTargetViews[i] && RenderTargetViews[i]->GetResource() == Resource) return true;
			}

			return false;
		}

		virtual ResourceUsageType GetResourceUsageType(RenderGraphResource* Resource) override
		{
			if (DepthStencilView && DepthStencilView->GetResource() == Resource)
			{
				return ResourceUsageType::DepthStencil;
			}

			for (UINT i = 0; i < 8; i++)
			{
				if (RenderTargetViews[i] && RenderTargetViews[i]->GetResource() == Resource) return ResourceUsageType::RenderTarget;
			}

			for (RenderGraphResourceView* ShaderResourceView : ShaderResourceViews)
			{
				if (ShaderResourceView->GetResource() == Resource) return ResourceUsageType::ShaderResource;
			}
		}

	private:
	
		RenderGraphResourceView *RenderTargetViews[8];
		RenderGraphResourceView *DepthStencilView;

		DynamicArray<RenderGraphResourceView*> ShaderResourceViews;
};