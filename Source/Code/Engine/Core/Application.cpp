#include "Application.h"

#include <Engine/Engine.h>

bool Application::AppExitFlag;
HWND Application::MainWindowHandle;

LRESULT CALLBACK Application::MainWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_CLOSE)
		Application::AppExitFlag = true;

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

void Application::StartApplication(const wchar_t* WindowTitle, HINSTANCE hInstance)
{
	BOOL Result;

	Result = AllocConsole();
	Result = SetConsoleTitle(WindowTitle);
	freopen("CONOUT$", "w", stdout);

	WNDCLASSEX WndClassEx;
	WndClassEx.cbClsExtra = 0;
	WndClassEx.cbSize = sizeof(WNDCLASSEX);
	WndClassEx.cbWndExtra = 0;
	WndClassEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClassEx.hIcon = NULL;
	WndClassEx.hIconSm = NULL;
	WndClassEx.hInstance = hInstance;
	WndClassEx.lpfnWndProc = &Application::MainWindowProc;
	WndClassEx.lpszClassName = L"MainWindowClass";
	WndClassEx.lpszMenuName = NULL;
	WndClassEx.style = 0;

	ATOM Atom = RegisterClassEx(&WndClassEx);

	DWORD WindowStyle = WS_POPUP;

	int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	Application::MainWindowHandle = CreateWindowEx(0, L"MainWindowClass", WindowTitle, WindowStyle, 0, 0, ScreenWidth, ScreenHeight, NULL, NULL, hInstance, NULL);

	Result = UpdateWindow(Application::MainWindowHandle);
	Result = ShowWindow(Application::MainWindowHandle, SW_SHOW);

	Application::AppExitFlag = false;

	Engine::GetEngine().InitEngine();
}

void Application::StopApplication()
{
	Engine::GetEngine().ShutdownEngine();

	BOOL Result;
	Result = DestroyWindow(Application::MainWindowHandle);

	Result = FreeConsole();
}

void Application::RunMainLoop()
{
	ULONGLONG CurrentTime = GetTickCount64(), NewTime;
	float DeltaTime;

	MSG Message;

	while (!Application::AppExitFlag)
	{
		while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
		{
			BOOL bResult = TranslateMessage(&Message);
			LRESULT lResult = DispatchMessage(&Message);
		}

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
			Application::AppExitFlag = true;

		NewTime = GetTickCount64();

		DeltaTime = float(NewTime - CurrentTime) / 1000.0f;

		Engine::GetEngine().TickEngine(DeltaTime);

		CurrentTime = NewTime;
	}
}