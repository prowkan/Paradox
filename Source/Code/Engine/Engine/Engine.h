#pragma once

#include <MultiThreading/MultiThreadingSystem.h>
#include <MemoryManager/MemoryManager.h>

#include <FileSystem/FileSystem.h>
#include <Input/InputSystem.h>
#include <Render/RenderSystem.h>

#include <ResourceManager/ResourceManager.h>

#include <Game/GameFramework.h>

class Engine
{
	public:

		static Engine& GetEngine() { return engine; }

		void InitEngine();
		void ShutdownEngine();
		void TickEngine(float DeltaTime);

		MultiThreadingSystem& GetMultiThreadingSystem() { return multiThreadingSystem; }
		MemoryManager& GetMemoryManager() { return memoryManager; }

		FileSystem& GetFileSystem() { return fileSystem; }
		InputSystem& GetInputSystem() { return inputSystem; }
		RenderSystem& GetRenderSystem() { return renderSystem; }

		ResourceManager& GetResourceManager() { return resourceManager; }

		GameFramework& GetGameFramework() { return gameFramework; }

	private:

		static Engine engine;

		MultiThreadingSystem multiThreadingSystem;
		MemoryManager memoryManager;

		FileSystem fileSystem;
		InputSystem inputSystem;
		RenderSystem renderSystem;

		ResourceManager resourceManager;

		GameFramework gameFramework;
};