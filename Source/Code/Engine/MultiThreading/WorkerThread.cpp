#include "WorkerThread.h"

#include <Engine/Engine.h>

#include "Task.h"

DWORD WINAPI WorkerThreadFunc(LPVOID lpThreadParameter)
{
	const UINT ThreadID = *(UINT*)lpThreadParameter;

	wchar_t ThreadName[256];
	wsprintf(ThreadName, L"Worker Thread %u", ThreadID + 1);

	SetThreadDescription(GetCurrentThread(), ThreadName);

	ThreadSafeQueue<Task*>& TaskQueue = Engine::GetEngine().GetMultiThreadingSystem().GetTaskQueue();
	HANDLE& TaskQueueEvent = Engine::GetEngine().GetMultiThreadingSystem().GetTaskQueueEvent();
	HANDLE& ThreadStopEvent = Engine::GetEngine().GetMultiThreadingSystem().GetThreadStopEvent(ThreadID);

	char OptickThreadName[256];
	sprintf(OptickThreadName, "Worker Thread %u", ThreadID + 1);

	OPTICK_THREAD(OptickThreadName);

	while (!MultiThreadingSystem::GetExitFlagValue())
	{
		Task *task;

		if (TaskQueue.Pop(task))
		{
			task->Execute(ThreadID);
		}
		else
		{
			DWORD WaitResult = WaitForSingleObject(TaskQueueEvent, 100);
		}
	}

	BOOL Result = SetEvent(ThreadStopEvent);

	return 0;
}