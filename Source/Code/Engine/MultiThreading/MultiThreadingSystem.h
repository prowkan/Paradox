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

		void AddTask(Task *task)
		{
			TaskQueue.Push(task);
			BOOL Result = SetEvent(TaskQueueEvent);
		}

		static bool GetExitFlagValue() { return WorkerThreadExitFlag.load(memory_order::memory_order_seq_cst); }

	private:


		static const int MAX_WORKER_THREADS = 16;
		HANDLE WorkerThreads[MAX_WORKER_THREADS];
		static atomic<bool> WorkerThreadExitFlag;
		UINT ThreadIndices[MAX_WORKER_THREADS];
		UINT WorkerThreadsCount;

		ThreadSafeQueue<Task*> TaskQueue;
		HANDLE TaskQueueEvent;
};