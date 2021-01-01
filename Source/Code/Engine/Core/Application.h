#pragma once

class Application
{
	public:

		static void StartApplication(const wchar_t* WindowTitle, HINSTANCE hInstance);
		static void RunMainLoop();
		static void StopApplication();

	private:

		static bool AppExitFlag;
		static HWND MainWindowHandle;

		static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
};