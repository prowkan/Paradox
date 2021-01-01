#include "Engine.h"

Engine Engine::engine;

void Engine::InitEngine()
{
	renderSystem.InitSystem();
}

void Engine::ShutdownEngine()
{
	renderSystem.ShutdownSystem();
}

void Engine::TickEngine(float DeltaTime)
{
	renderSystem.TickSystem(DeltaTime);
}