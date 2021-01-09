#pragma once

#include <MemoryManager/MemoryManager.h>

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

		MemoryManager& GetMemoryManager() { return memoryManager; }

		InputSystem& GetInputSystem() { return inputSystem; }
		RenderSystem& GetRenderSystem() { return renderSystem; }

		ResourceManager& GetResourceManager() { return resourceManager; }

		GameFramework& GetGameFramework() { return gameFramework; }

	private:

		static Engine engine;

		MemoryManager memoryManager;

		InputSystem inputSystem;
		RenderSystem renderSystem;

		ResourceManager resourceManager;

		GameFramework gameFramework;
};