#pragma once

#include <Containers/StaticReference.h>

#include <MultiThreading/MultiThreadingSystem.h>
#include <MemoryManager/MemoryManager.h>

#include <Config/ConfigSystem.h>
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

		ConfigSystem& GetConfigSystem() { return configSystem; }
		FileSystem& GetFileSystem() { return fileSystem; }
		InputSystem& GetInputSystem() { return inputSystem; }
		RenderSystem& GetRenderSystem() { return renderSystem; }

		ResourceManager& GetResourceManager() { return resourceManager; }

		GameFramework& GetGameFramework() { return gameFramework; }

	private:

		static StaticReference<Engine> engine;

		StaticReference<MultiThreadingSystem> multiThreadingSystem;
		StaticReference<MemoryManager> memoryManager;

		StaticReference<ConfigSystem> configSystem;
		StaticReference<FileSystem> fileSystem;
		StaticReference<InputSystem> inputSystem;
		StaticReference<RenderSystem> renderSystem;

		StaticReference<ResourceManager> resourceManager;

		StaticReference<GameFramework> gameFramework;
};