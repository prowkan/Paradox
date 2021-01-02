#include "Engine.h"

Engine Engine::engine;

void Engine::InitEngine()
{
	renderSystem.InitSystem();
	gameFramework.InitFramework();
}

void Engine::ShutdownEngine()
{
	gameFramework.ShutdownFramework();
	renderSystem.ShutdownSystem();
}

void Engine::TickEngine(float DeltaTime)
{
	gameFramework.TickFramework(DeltaTime);
	renderSystem.TickSystem(DeltaTime);
}