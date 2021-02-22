#pragma once

class String;

class Application
{
	public:

		static void StartApplication(const String& WindowTitle, HINSTANCE hInstance);
		static void StopApplication();
		static void RunMainLoop();

		static HWND GetMainWindowHandle() { return MainWindowHandle; }

	private:

		static bool AppExitFlag;
		static HWND MainWindowHandle;

		static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
		static LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo);

		static atomic<bool> ExceptionFlag;
};