// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "WorkerThread.h"

#include <Engine/Engine.h>

#include "Task.h"

DWORD WINAPI WorkerThreadFunc(LPVOID lpThreadParameter)
{
	const UINT ThreadID = *(UINT*)lpThreadParameter; //-V206

	char16_t ThreadName[256];
	wsprintf((wchar_t*)ThreadName, (const wchar_t*)u"Worker Thread %u", ThreadID + 1);

	SetThreadDescription(GetCurrentThread(), (const wchar_t*)ThreadName);

	ThreadSafeQueue<Task*>& TaskQueue = Engine::GetEngine().GetMultiThreadingSystem().GetTaskQueue();
	HANDLE& TaskQueueEvent = TaskQueue.GetQueueEvent();

	char OptickThreadName[256];
	sprintf(OptickThreadName, "Worker Thread %u", ThreadID + 1);

	OPTICK_THREAD(OptickThreadName);

	while (!MultiThreadingSystem::GetExitFlagValue())
	{
		Task *task;

		if (TaskQueue.Pop(task))
		{
			task->Execute(ThreadID);
			task->Finish();
		}
		else
		{
			DWORD WaitResult = WaitForSingleObject(TaskQueueEvent, 100);
		}
	}

	return 0;
}