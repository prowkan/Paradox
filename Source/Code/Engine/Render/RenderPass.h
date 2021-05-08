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

		void AddShaderResource(RenderGraphResourceView* ResourceView)
		{
			RenderPassShaderResources.Add(ResourceView);
		}

		void AddRenderTarget(RenderGraphResourceView* ResourceView)
		{
			RenderPassShaderResources.Add(ResourceView);
		}

		void AddDepthStencil(RenderGraphResourceView* ResourceView)
		{
			RenderPassShaderResources.Add(ResourceView);
		}

		void AddUnorderedAccess(RenderGraphResourceView* ResourceView)
		{
			RenderPassShaderResources.Add(ResourceView);
		}

		template<typename T>
		void SetExecutionCallBack(const T& CallBack)
		{
			new (CallBackCallerStorage) RenderPassCallBackCaller<T>(CallBack);
		}

		void ExecutePass()
		{
			CallBackCaller->Call();
		}

	private:

		String Name;

		DynamicArray<RenderGraphResourceView*> RenderPassShaderResources;
		DynamicArray<RenderGraphResourceView*> RenderPassRenderTargets;
		DynamicArray<RenderGraphResourceView*> RenderPassDepthStencils;
		DynamicArray<RenderGraphResourceView*> RenderPassUnorderedAccesses;

		RenderPassCallBackCallerBase *CallBackCaller;
		BYTE CallBackCallerStorage[1024];
};