#pragma once

#include <Containers/String.h>
#include <Containers/DynamicArray.h>

class RenderGraphResource;
class RenderGraphResourceView;

class RenderPassCallBackCallerBase
{
	public:

		virtual void Call() = 0;
};

template<typename CallBackType>
class RenderPassCallBackCaller : public RenderPassCallBackCallerBase
{
	private:

		CallBackType CallBack;

	public:

		RenderPassCallBackCaller(const CallBackType& CallBack) : CallBack(CallBack) {}

		virtual void Call() override
		{
			CallBack();
		}
};

class RenderPass
{
	friend class RenderGraph;

	public:

		RenderPass()
		{
			CallBackCaller = (RenderPassCallBackCallerBase*)CallBackCallerStorage;
		}

		/*void AddShaderResource(RenderGraphResourceView* ResourceView)
		{
			RenderPassShaderResources.Add(ResourceView);
		}

		void AddRenderTarget(RenderGraphResourceView* ResourceView)
		{
			RenderPassRenderTargets.Add(ResourceView);
		}

		void AddDepthStencil(RenderGraphResourceView* ResourceView)
		{
			RenderPassDepthStencils.Add(ResourceView);
		}

		void AddUnorderedAccess(RenderGraphResourceView* ResourceView)
		{
			RenderPassUnorderedAccesses.Add(ResourceView);
		}*/

		template<typename T>
		void SetExecutionCallBack(const T& CallBack)
		{
			new (CallBackCallerStorage) RenderPassCallBackCaller<T>(CallBack);
		}

		void ExecutePass()
		{
			CallBackCaller->Call();
		}

		enum class ResourceUsageType { ShaderResource, RenderTarget, DepthStencil, UnorderedAccess, ResolveInput, ResolveOutput };

		virtual bool IsResourceUsedInRenderPass(RenderGraphResource* Resource) = 0;
		virtual bool IsResourceReadInRenderPass(RenderGraphResource* Resource) = 0;
		virtual bool IsResourceWrittenInRenderPass(RenderGraphResource* Resource) = 0;

		virtual ResourceUsageType GetResourceUsageType(RenderGraphResource* Resource) = 0;

	private:

		String Name;

		DynamicArray<RenderGraphResourceView*> RenderPassShaderResources;
		DynamicArray<RenderGraphResourceView*> RenderPassRenderTargets;
		DynamicArray<RenderGraphResourceView*> RenderPassDepthStencils;
		DynamicArray<RenderGraphResourceView*> RenderPassUnorderedAccesses;

		struct ResourceBarrier
		{
			RenderGraphResource *Resource;
			UINT SubResourceIndex;
			D3D12_RESOURCE_STATES OldState;
			D3D12_RESOURCE_STATES NewState;
		};

		DynamicArray<ResourceBarrier> ResourceBarriers;

		RenderPassCallBackCallerBase *CallBackCaller;
		BYTE CallBackCallerStorage[1024];
};