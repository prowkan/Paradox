// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Engine.h"

#include <FlashUI/SWFFile.h>
#include <FlashUI/SWFParser.h>

Engine Engine::engine;

void Engine::InitEngine()
{
	SWFFile File;
	File.Open(u"GameContent/UI/Test.swf");
	SWFParser::ParseFile(File);
	File.Close();

	multiThreadingSystem.InitSystem();
	memoryManager.InitManager();
	multiThreadingSystem.InitSystem();
	inputSystem.InitSystem();
	renderSystem.InitSystem();
	gameFramework.InitFramework();
}

void Engine::ShutdownEngine()
{
	inputSystem.ShutdownSystem();
	gameFramework.ShutdownFramework();
	renderSystem.ShutdownSystem();
	multiThreadingSystem.ShutdownSystem();
	memoryManager.ShutdownManager();
}

void Engine::TickEngine(float DeltaTime)
{
	OPTICK_FRAME("Main Thread")

	inputSystem.TickSystem(DeltaTime);
	gameFramework.TickFramework(DeltaTime);
	renderSystem.TickSystem(DeltaTime);
}