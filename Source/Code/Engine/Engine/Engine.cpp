// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Engine.h"

#include <FlashUI/SWFMovie.h>
#include <FlashUI/SWFStream.h>

StaticReference<Engine> Engine::engine;

void Engine::InitEngine()
{
	SystemMemoryAllocator::InitAllocator();

	SWFStream Stream = SWFStream::CreateFromFile(u"GameContent/UI/UI_MainMenu.swf");
	SWFMovie Movie(Stream);

	memoryManager.CreateInstance();
	configSystem.CreateInstance();
	multiThreadingSystem.CreateInstance();
	inputSystem.CreateInstance();
	renderSystem.CreateInstance();
	fileSystem.CreateInstance();
	gameFramework.CreateInstance();
	resourceManager.CreateInstance();

	memoryManager->InitManager();
	configSystem->InitSystem();
	multiThreadingSystem->InitSystem();
	fileSystem->InitSystem();
	inputSystem->InitSystem();
	renderSystem->InitSystem();
	gameFramework->InitFramework();
}

void Engine::ShutdownEngine()
{
	inputSystem->ShutdownSystem();
	gameFramework->ShutdownFramework();
	renderSystem->ShutdownSystem();
	fileSystem->ShutdownSystem();
	multiThreadingSystem->ShutdownSystem();
	memoryManager->ShutdownManager();
	configSystem->ShutdownSystem();

	memoryManager.DestroyInstance();
	configSystem.DestroyInstance();
	multiThreadingSystem.DestroyInstance();
	inputSystem.DestroyInstance();
	renderSystem.DestroyInstance();
	fileSystem.DestroyInstance();
	gameFramework.DestroyInstance();
	resourceManager.DestroyInstance();
}

void Engine::TickEngine(float DeltaTime)
{
	OPTICK_FRAME("Main Thread")

	inputSystem->TickSystem(DeltaTime);
	gameFramework->TickFramework(DeltaTime);
	renderSystem->TickSystem(DeltaTime);
}