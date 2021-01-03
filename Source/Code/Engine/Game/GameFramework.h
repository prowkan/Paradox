#pragma once

#include "Camera.h"
#include "World.h"

class GameFramework
{
	public:

		void InitFramework();
		void ShutdownFramework();
		void TickFramework(float DeltaTime);

		Camera& GetCamera() { return camera; }
		World& GetWorld() { return world; }

	private:

		Camera camera;
		World world;
};