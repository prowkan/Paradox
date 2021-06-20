#pragma once

class PCWindowsPlatformThread
{
	public:

		using WindowThreadRoutineType = DWORD(WINAPI *)(LPVOID);

		static PCWindowsPlatformThread Create(WindowThreadRoutineType RoutinePointer, void* DataPointer)
		{
			PCWindowsPlatformThread Thread;
			Thread.WindowsThread = CreateThread(NULL, 0, RoutinePointer, DataPointer, 0, NULL); //-V513

			return Thread;
		}

		PCWindowsPlatformThread()
		{
			WindowsThread = INVALID_HANDLE_VALUE;
		}

		void WaitForFinishAndDestroy()
		{
			DWORD WaitResult = WaitForSingleObject(WindowsThread, INFINITE);
			BOOL Result = CloseHandle(WindowsThread);
			WindowsThread = INVALID_HANDLE_VALUE;
		}

	private:

		HANDLE WindowsThread;
};