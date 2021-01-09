#include "Engine.h"

Engine Engine::engine;

void Engine::InitEngine()
{
	memoryManager.InitManager();
	inputSystem.InitSystem();
	renderSystem.InitSystem();
	gameFramework.InitFramework();
}

void Engine::ShutdownEngine()
{
	inputSystem.ShutdownSystem();
	gameFramework.ShutdownFramework();
	renderSystem.ShutdownSystem();
	memoryManager.ShutdownManager();
}

void Engine::TickEngine(float DeltaTime)
{
	OPTICK_FRAME("Main Thread")

	inputSystem.TickSystem(DeltaTime);
	gameFramework.TickFramework(DeltaTime);
	renderSystem.TickSystem(DeltaTime);
}