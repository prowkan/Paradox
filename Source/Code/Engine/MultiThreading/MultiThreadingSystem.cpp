#include "MultiThreadingSystem.h"

#include "WorkerThread.h"
#include "RenderThread.h"

atomic<bool> MultiThreadingSystem::WorkerThreadExitFlag;

void MultiThreadingSystem::InitSystem()
{
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);

	WorkerThreadsCount = SystemInfo.dwNumberOfProcessors - 1;

	WorkerThreadExitFlag.store(false, memory_order::memory_order_seq_cst);

	for (UINT i = 0; i < WorkerThreadsCount; i++)
	{
		ThreadIndices[i] = i;
		WorkerThreads[i] = CreateThread(NULL, 0, &WorkerThreadFunc, &ThreadIndices[i], 0, NULL);
	}

	RenderThread = CreateThread(NULL, 0, &RenderThreadFunc, NULL, 0, NULL);
}

void MultiThreadingSystem::ShutdownSystem()
{
	WorkerThreadExitFlag.store(true, memory_order::memory_order_seq_cst);

	for (UINT i = 0; i < WorkerThreadsCount; i++)
	{
		DWORD WaitResult = WaitForSingleObject(WorkerThreads[i], INFINITE);

		BOOL Result = CloseHandle(WorkerThreads[i]);
	
		WorkerThreads[i] = INVALID_HANDLE_VALUE;
	}

	DWORD WaitResult = WaitForSingleObject(RenderThread, INFINITE);

	BOOL Result = CloseHandle(RenderThread);
	RenderThread = INVALID_HANDLE_VALUE;
}