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

		static bool IsEditor() { return EditorFlag; }

		static void SetLevelRenderCanvasHandle(HWND NewLevelRenderCanvasHandle) { LevelRenderCanvasHandle = NewLevelRenderCanvasHandle; }
		static void SetAppExitFlag(bool Value) { AppExitFlag = Value; }

		static UINT EditorViewportWidth;
		static UINT EditorViewportHeight;
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