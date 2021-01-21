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

LONG WINAPI Application::UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo)
{
	char16_t ErrorMessageBuffer[1024];
	char16_t ExceptionCodeBuffer[256];

	switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_ACCESS_VIOLATION");
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_DATATYPE_MISALIGNMENT");
			break;
		case EXCEPTION_BREAKPOINT:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_BREAKPOINT");
			break;
		case EXCEPTION_SINGLE_STEP:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_SINGLE_STEP");
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_FLT_DENORMAL_OPERAND");
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_FLT_DIVIDE_BY_ZERO");
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_FLT_INEXACT_RESULT");
			break;
		case EXCEPTION_FLT_INVALID_OPERATION:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_FLT_INVALID_OPERATION");
			break;
		case EXCEPTION_FLT_OVERFLOW:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_FLT_OVERFLOW");
			break;
		case EXCEPTION_FLT_STACK_CHECK:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_FLT_STACK_CHECK");
			break;
		case EXCEPTION_FLT_UNDERFLOW:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_FLT_UNDERFLOW");
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_INT_DIVIDE_BY_ZERO");
			break;
		case EXCEPTION_INT_OVERFLOW:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_INT_OVERFLOW");
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_PRIV_INSTRUCTION");
			break;
		case EXCEPTION_IN_PAGE_ERROR:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_IN_PAGE_ERROR");
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_ILLEGAL_INSTRUCTION");
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_NONCONTINUABLE_EXCEPTION");
			break;
		case EXCEPTION_STACK_OVERFLOW:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_STACK_OVERFLOW");
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_INVALID_DISPOSITION");
			break;
		case EXCEPTION_GUARD_PAGE:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_GUARD_PAGE");
			break;
		case EXCEPTION_INVALID_HANDLE:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"EXCEPTION_INVALID_HANDLE");
			break;
		default:
			wcscpy((wchar_t*)ExceptionCodeBuffer, (const wchar_t*)u"неизвестный код");
			break;
	}

	wsprintf((wchar_t*)ErrorMessageBuffer, (const wchar_t*)u"Поймано необработанное исключение.\r\nКод исключения: 0x%08X (%s)\r\nАдрес исключения: 0x%p", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionCodeBuffer, ExceptionInfo->ExceptionRecord->ExceptionAddress);

	int IntResult = MessageBox(NULL, (const wchar_t*)ErrorMessageBuffer, (const wchar_t*)u"Критическая ошибка", MB_OK | MB_ICONERROR);

	char16_t MiniDumpFileName[256];
	char16_t ModuleFileName[256];

	DWORD DResult = GetModuleFileName(NULL, (wchar_t*)ModuleFileName, 256);

	SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);

	wsprintf((wchar_t*)MiniDumpFileName, (const wchar_t*)u"%s_Crash_%02d-%02d-%02d_%02d-%02d-%04d.dmp", ModuleFileName, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear);

	HANDLE MiniDumpFile = CreateFile((wchar_t*)MiniDumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

	MINIDUMP_EXCEPTION_INFORMATION MiniDumpExceptionInformation;
	MiniDumpExceptionInformation.ClientPointers = TRUE;
	MiniDumpExceptionInformation.ExceptionPointers = ExceptionInfo;
	MiniDumpExceptionInformation.ThreadId = GetCurrentThreadId();

	BOOL Result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), MiniDumpFile, MINIDUMP_TYPE::MiniDumpWithDataSegs, &MiniDumpExceptionInformation, NULL, NULL);

	return EXCEPTION_CONTINUE_SEARCH;
}

void Application::StartApplication(const char16_t* WindowTitle, HINSTANCE hInstance)
{
	LPTOP_LEVEL_EXCEPTION_FILTER TopLevelExceptionFilter = SetUnhandledExceptionFilter(&Application::UnhandledExceptionFilter);

	BOOL Result;

	Result = AllocConsole();
	Result = SetConsoleTitle((wchar_t*)WindowTitle);
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
	WndClassEx.lpszClassName = (const wchar_t*)u"MainWindowClass";
	WndClassEx.lpszMenuName = NULL;
	WndClassEx.style = 0;

	ATOM Atom = RegisterClassEx(&WndClassEx);

	DWORD WindowStyle = WS_POPUP;

	int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	Application::MainWindowHandle = CreateWindowEx(0, (const wchar_t*)u"MainWindowClass", (const wchar_t*)WindowTitle, WindowStyle, 0, 0, ScreenWidth, ScreenHeight, NULL, NULL, hInstance, NULL);

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