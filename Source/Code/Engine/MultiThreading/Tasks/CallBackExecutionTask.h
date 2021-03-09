#pragma once

#include "../Task.h"

template<typename CallBackType, typename ArgType1, typename ArgType2>
class CallBackExecutionTask : public Task
{
	public:

		CallBackExecutionTask(CallBackType CallBack, ArgType1 ArgValue1, ArgType2 ArgValue2) : CallBack(CallBack)
		{
			this->ArgValue1 = ArgValue1;
			this->ArgValue2 = ArgValue2;
		}

		virtual void Execute(const UINT ThreadID) override
		{
			CallBack(ThreadID, ArgValue1, ArgValue2);
		}

	private:

		CallBackType CallBack;

		ArgType1 ArgValue1;
		ArgType2 ArgValue2;
};
