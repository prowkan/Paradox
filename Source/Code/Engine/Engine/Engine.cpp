// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Engine.h"

Engine Engine::engine;

void Engine::InitEngine()
{
	memoryManager.InitManager();
	configSystem.InitSystem();
	multiThreadingSystem.InitSystem();
	fileSystem.InitSystem();
	inputSystem.InitSystem();
	renderSystem.InitSystem();
	gameFramework.InitFramework();
}

void Engine::ShutdownEngine()
{
	inputSystem.ShutdownSystem();
	gameFramework.ShutdownFramework();
	renderSystem.ShutdownSystem();
	fileSystem.ShutdownSystem();
	multiThreadingSystem.ShutdownSystem();
	memoryManager.ShutdownManager();
	configSystem.ShutdownSystem();
}

void Engine::TickEngine(float DeltaTime)
{
	OPTICK_FRAME("Main Thread")

	inputSystem.TickSystem(DeltaTime);
	gameFramework.TickFramework(DeltaTime);
	renderSystem.TickSystem(DeltaTime);
}