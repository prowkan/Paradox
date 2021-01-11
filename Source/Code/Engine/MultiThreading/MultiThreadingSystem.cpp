#include "MultiThreadingSystem.h"

#include "WorkerThread.h"

void MultiThreadingSystem::InitSystem()
{
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);

	WorkerThreadsCount = SystemInfo.dwNumberOfProcessors;

	for (UINT i = 0; i < WorkerThreadsCount; i++)
	{
		ThreadIndices[i] = i;
		WorkerThreads[i] = CreateThread(NULL, 0, &WorkerThreadFunc, &ThreadIndices[i], 0, NULL);
	}
}

void MultiThreadingSystem::ShutdownSystem()
{

}