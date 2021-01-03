#include "Engine.h"

Engine Engine::engine;

void Engine::InitEngine()
{
	inputSystem.InitSystem();
	renderSystem.InitSystem();
	gameFramework.InitFramework();
}

void Engine::ShutdownEngine()
{
	inputSystem.ShutdownSystem();
	gameFramework.ShutdownFramework();
	renderSystem.ShutdownSystem();
}

void Engine::TickEngine(float DeltaTime)
{
	inputSystem.TickSystem(DeltaTime);
	gameFramework.TickFramework(DeltaTime);
	renderSystem.TickSystem(DeltaTime);
}