#include "RenderThread.h"

#include <Engine/Engine.h>

DWORD WINAPI RenderThreadFunc(LPVOID lpThreadParameter)
{
	SetThreadDescription(GetCurrentThread(), L"Render Thread");

	OPTICK_THREAD("Render Thread");

	while (!MultiThreadingSystem::GetExitFlagValue())
	{
		Engine::GetEngine().GetRenderSystem().RenderThreadFunc();
	}

	return 0;
}