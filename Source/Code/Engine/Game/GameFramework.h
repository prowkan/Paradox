#pragma once

#include "World.h"

class GameFramework
{
	public:

		void InitFramework();
		void ShutdownFramework();
		void TickFramework(float DeltaTime);

		World& GetWorld() { return world; }

	private:

		World world;
};