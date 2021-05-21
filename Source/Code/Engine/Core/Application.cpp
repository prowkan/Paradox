// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Application.h"

#include <Engine/Engine.h>
#include <Containers/FixedSizeString.h>
#include <Containers/String.h>

bool Application::EditorFlag;
bool Application::AppExitFlag;
HWND Application::MainWindowHandle;
HWND Application::LevelRenderCanvasHandle;
atomic<bool> Application::ExceptionFlag;
UINT Application::EditorViewportWidth;
UINT Application::EditorViewportHeight;

LRESULT CALLBACK Application::MainWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_CLOSE)
		Application::AppExitFlag = true;

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

LONG WINAPI Application::UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo)
{
	if (Application::ExceptionFlag.load(memory_order::memory_order_seq_cst) == true)
	{
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	
	Application::ExceptionFlag.store(true, memory_order::memory_order_seq_cst);

	FixedSizeString<1024> ErrorMessageString;
	FixedSizeString<256> ExceptionCodeString;

	switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_ACCESS_VIOLATION"));
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_DATATYPE_MISALIGNMENT"));
			break;
		case EXCEPTION_BREAKPOINT:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_BREAKPOINT"));
			break;
		case EXCEPTION_SINGLE_STEP:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_SINGLE_STEP"));
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_FLT_DENORMAL_OPERAND"));
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_FLT_DIVIDE_BY_ZERO"));
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_FLT_INEXACT_RESULT"));
			break;
		case EXCEPTION_FLT_INVALID_OPERATION:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_FLT_INVALID_OPERATION"));
			break;
		case EXCEPTION_FLT_OVERFLOW:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_FLT_OVERFLOW"));
			break;
		case EXCEPTION_FLT_STACK_CHECK:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_FLT_STACK_CHECK"));
			break;
		case EXCEPTION_FLT_UNDERFLOW:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_FLT_UNDERFLOW"));
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_INT_DIVIDE_BY_ZERO"));
			break;
		case EXCEPTION_INT_OVERFLOW:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_INT_OVERFLOW"));
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_PRIV_INSTRUCTION"));
			break;
		case EXCEPTION_IN_PAGE_ERROR:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_IN_PAGE_ERROR"));
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_ILLEGAL_INSTRUCTION"));
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_NONCONTINUABLE_EXCEPTION"));
			break;
		case EXCEPTION_STACK_OVERFLOW:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_STACK_OVERFLOW"));
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_INVALID_DISPOSITION"));
			break;
		case EXCEPTION_GUARD_PAGE:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_GUARD_PAGE"));
			break;
		case EXCEPTION_INVALID_HANDLE:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"EXCEPTION_INVALID_HANDLE"));
			break;
		default:
			wcscpy(ExceptionCodeString, (const wchar_t*)(u"неизвестный код"));
			break;
	}

	wsprintf(ErrorMessageString, (const wchar_t*)(u"Поймано необработанное исключение.\r\nКод исключения: 0x%08X (%s)\r\nАдрес исключения: 0x%p"), ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionCodeString, ExceptionInfo->ExceptionRecord->ExceptionAddress);

	int IntResult = MessageBox(NULL, ErrorMessageString, (const wchar_t*)(u"Критическая ошибка"), MB_OK | MB_ICONERROR);

	FixedSizeString<256> MiniDumpFileName;
	FixedSizeString<256> ModuleFileName;

	DWORD DResult = GetModuleFileName(NULL, ModuleFileName, (DWORD)ModuleFileName.GetStorageLength());

	SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);

	wsprintf(MiniDumpFileName, (const wchar_t*)(u"%s_Crash_%02d-%02d-%02d_%02d-%02d-%04d.dmp"), ModuleFileName, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear);

	HANDLE MiniDumpFile = CreateFile(MiniDumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

	MINIDUMP_EXCEPTION_INFORMATION MiniDumpExceptionInformation;
	MiniDumpExceptionInformation.ClientPointers = TRUE;
	MiniDumpExceptionInformation.ExceptionPointers = ExceptionInfo;
	MiniDumpExceptionInformation.ThreadId = GetCurrentThreadId();

	BOOL Result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), MiniDumpFile, MINIDUMP_TYPE::MiniDumpWithDataSegs, &MiniDumpExceptionInformation, NULL, NULL);

	return EXCEPTION_CONTINUE_SEARCH;
}

void Application::StartApplication(const char16_t* WindowTitle, HINSTANCE hInstance)
{
	Application::EditorFlag = false;

	LPTOP_LEVEL_EXCEPTION_FILTER TopLevelExceptionFilter = SetUnhandledExceptionFilter(&Application::UnhandledExceptionFilter);

	BOOL Result;

	Result = AllocConsole();
	Result = SetConsoleTitle((const wchar_t*)WindowTitle);
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
	WndClassEx.lpszClassName = (const wchar_t*)(u"MainWindowClass");
	WndClassEx.lpszMenuName = NULL;
	WndClassEx.style = 0;

	ATOM Atom = RegisterClassEx(&WndClassEx);

	//DWORD WindowStyle = WS_POPUP;

	DWORD WindowStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	int WindowWidth = 1280;
	int WindowHeight = 720;

	int WindowLeft = ScreenWidth / 2 - WindowWidth / 2;
	int WindowTop = ScreenHeight / 2 - WindowHeight / 2;

	RECT WindowRect;
	WindowRect.bottom = WindowTop + WindowHeight;
	WindowRect.left = WindowLeft;
	WindowRect.right = WindowLeft + WindowWidth;
	WindowRect.top = WindowTop;

	Result = AdjustWindowRect(&WindowRect, WindowStyle, FALSE);

	WindowWidth = WindowRect.right - WindowRect.left;
	WindowHeight = WindowRect.bottom - WindowRect.top;

	Application::MainWindowHandle = CreateWindowEx(0, (const wchar_t*)(u"MainWindowClass"), (const wchar_t*)WindowTitle, WindowStyle, WindowLeft, WindowTop, WindowWidth, WindowHeight, NULL, NULL, hInstance, NULL);

	Result = UpdateWindow(Application::MainWindowHandle);
	Result = ShowWindow(Application::MainWindowHandle, SW_SHOW);

	Application::AppExitFlag = false;
	Application::ExceptionFlag.store(false, memory_order::memory_order_seq_cst);

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

#if WITH_EDITOR
void Application::EditorStartApplication()
{
	Application::EditorFlag = true;

	//LPTOP_LEVEL_EXCEPTION_FILTER TopLevelExceptionFilter = SetUnhandledExceptionFilter(&Application::UnhandledExceptionFilter);

	/*BOOL Result;

	Result = AllocConsole();
	//Result = SetConsoleTitle((wchar_t*)"");
	freopen("CONOUT$", "w", stdout);*/

	Application::AppExitFlag = false;
	//Application::ExceptionFlag.store(false, memory_order::memory_order_seq_cst);

	Engine::GetEngine().InitEngine();
}

void Application::EditorStopApplication()
{
	Engine::GetEngine().ShutdownEngine();

	/*BOOL Result;

	Result = FreeConsole();*/
}

void Application::EditorRunMainLoop()
{
	ULONGLONG CurrentTime = GetTickCount64(), NewTime;
	float DeltaTime;

	while (!Application::AppExitFlag)
	{
		NewTime = GetTickCount64();

		DeltaTime = float(NewTime - CurrentTime) / 1000.0f;

		Engine::GetEngine().TickEngine(DeltaTime);

		CurrentTime = NewTime;
	}
}
#endif