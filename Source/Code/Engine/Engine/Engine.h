#pragma once

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

		RenderSystem& GetRenderSystem() { return renderSystem; }

		ResourceManager& GetResourceManager() { return resourceManager; }

		GameFramework& GetGameFramework() { return gameFramework; }

	private:

		static Engine engine;

		RenderSystem renderSystem;

		ResourceManager resourceManager;

		GameFramework gameFramework;
};