#pragma once

#include "ThreadSafeQueue.h"

class Task;

class MultiThreadingSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();

		ThreadSafeQueue<Task*>& GetTaskQueue() { return TaskQueue; }
		HANDLE& GetTaskQueueEvent() { return TaskQueueEvent; }

	private:

		static const int MAX_WORKER_THREADS = 16;
		HANDLE WorkerThreads[MAX_WORKER_THREADS];
		UINT ThreadIndices[MAX_WORKER_THREADS];
		UINT WorkerThreadsCount;

		ThreadSafeQueue<Task*> TaskQueue;
		HANDLE TaskQueueEvent;
};