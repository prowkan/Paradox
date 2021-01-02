#pragma once

#include "World.h"

class GameFramework
{
	public:

		void InitFramework();
		void ShutdownFramework();
		void TickFramework(float DeltaTime);

	private:

		World world;
};