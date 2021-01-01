#pragma once

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

		RenderSystem renderSystem;
};