#pragma once

#include <Game/GameFramework.h>
#include <Render/RenderSystem.h>

class Engine
{
	public:

		static Engine& GetEngine() { return engine; }

		void InitEngine();
		void ShutdownEngine();
		void TickEngine(float DeltaTime);

	private:

		static Engine engine;

		GameFramework gameFramework;
		RenderSystem renderSystem;
};