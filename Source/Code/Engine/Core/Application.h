#pragma once

class Application
{
	public:

		static void StartApplication(const char16_t* WindowTitle, HINSTANCE hInstance);
		static void StopApplication();
		static void RunMainLoop();

		static HWND GetMainWindowHandle() { return MainWindowHandle; }

#if WITH_EDITOR
		static void EditorStartApplication();
		static void EditorStopApplication();
		static void EditorRunMainLoop();

		static HWND GetLevelRenderCanvasHandle() { return LevelRenderCanvasHandle; }

		static bool IsEditor() { return Application::EditorFlag; }

		static void SetLevelRenderCanvasHandle(HWND LevelRenderCanvasHandle) { Application::LevelRenderCanvasHandle = LevelRenderCanvasHandle; }
		static void SetAppExitFlag(bool Value) { Application::AppExitFlag = Value; }
#endif

	private:

#if WITH_EDITOR
		static bool EditorFlag;
		static HWND LevelRenderCanvasHandle;
#endif
		static bool AppExitFlag;
		static HWND MainWindowHandle;

		static LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
		static LONG WINAPI UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo);

		static atomic<bool> ExceptionFlag;
};