#include "MultiThreadingSystem.h"

#include "WorkerThread.h"

atomic<bool> MultiThreadingSystem::WorkerThreadExitFlag;

void MultiThreadingSystem::InitSystem()
{
	TaskQueueEvent = CreateEvent(NULL, FALSE, FALSE, L"TaskQueueEvent");

	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);

	WorkerThreadsCount = SystemInfo.dwNumberOfProcessors;

	WorkerThreadExitFlag.store(false, memory_order::memory_order_seq_cst);

	for (UINT i = 0; i < WorkerThreadsCount; i++)
	{
		ThreadIndices[i] = i;
		WorkerThreads[i] = CreateThread(NULL, 0, &WorkerThreadFunc, &ThreadIndices[i], 0, NULL);

		wchar_t ThreadStopEventName[256];
		wsprintf(ThreadStopEventName, L"ThreadStopEvent_%u", i);

		ThreadStopEvents[i] = CreateEvent(NULL, FALSE, FALSE, ThreadStopEventName);
	}
}

void MultiThreadingSystem::ShutdownSystem()
{
	WorkerThreadExitFlag.store(true, memory_order::memory_order_seq_cst);

	for (UINT i = 0; i < WorkerThreadsCount; i++)
	{
		//BOOL Result = SetEvent(TaskQueueEvent);
		DWORD WaitResult = WaitForSingleObject(ThreadStopEvents[i], INFINITE);

		BOOL Result;

		Result = CloseHandle(WorkerThreads[i]);
		Result = CloseHandle(ThreadStopEvents[i]);

		WorkerThreads[i] = INVALID_HANDLE_VALUE;
		ThreadStopEvents[i] = INVALID_HANDLE_VALUE;
	}

	BOOL Result = CloseHandle(TaskQueueEvent);
	TaskQueueEvent = INVALID_HANDLE_VALUE;
}