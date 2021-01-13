#include "Engine.h"

Engine Engine::engine;

void Engine::InitEngine()
{
	renderSystem.InitSystem();
	multiThreadingSystem.InitSystem();
	memoryManager.InitManager();
	inputSystem.InitSystem();
	gameFramework.InitFramework();
}

void Engine::ShutdownEngine()
{
	inputSystem.ShutdownSystem();
	gameFramework.ShutdownFramework();
	multiThreadingSystem.ShutdownSystem();
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