#include "WorkerThread.h"

DWORD WINAPI WorkerThreadFunc(LPVOID lpThreadParameter)
{
	UINT ThreadID = *(UINT*)lpThreadParameter;

	wchar_t ThreadName[256];
	wsprintf(ThreadName, L"Worker Thread %u", ThreadID);

	SetThreadDescription(GetCurrentThread(), ThreadName);

	OPTICK_THREAD("Worker Thread");

	while (true)
	{
		Sleep(10);
	}

	return 0;
}