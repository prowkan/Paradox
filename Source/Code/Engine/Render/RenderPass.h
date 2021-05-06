#pragma once

#include <Containers/String.h>
#include <Containers/DynamicArray.h>

class RenderGraphResource;

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

		void AddInput(RenderGraphResource* Resource)
		{
			RenderPassInputs.Add(Resource);
		}

		void AddOutput(RenderGraphResource* Resource)
		{
			RenderPassOutputs.Add(Resource);
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

		DynamicArray<RenderGraphResource*> RenderPassInputs;
		DynamicArray<RenderGraphResource*> RenderPassOutputs;

		RenderPassCallBackCallerBase *CallBackCaller;
		BYTE CallBackCallerStorage[1024];
};