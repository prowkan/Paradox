// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <../Engine/Core/Application.h>

extern "C" __declspec(dllexport) void StartApplication()
{
	Application::EditorStartApplication();
}

extern "C" __declspec(dllexport) void StopApplication()
{
	Application::EditorStopApplication();
}

extern "C" __declspec(dllexport) void RunMainLoop()
{
	Application::EditorRunMainLoop();
}

extern "C" __declspec(dllexport) void SetLevelRenderCanvasHandle(HWND LevelRenderCanvasHandle)
{
	Application::SetLevelRenderCanvasHandle(LevelRenderCanvasHandle);
}

extern "C" __declspec(dllexport) void SetAppExitFlag(bool Value)
{
	Application::SetAppExitFlag(Value);
}