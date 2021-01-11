#pragma once

class MultiThreadingSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();

	private:

		static const int MAX_WORKER_THREADS = 16;
		HANDLE WorkerThreads[MAX_WORKER_THREADS];
		UINT ThreadIndices[MAX_WORKER_THREADS];
		UINT WorkerThreadsCount;
};