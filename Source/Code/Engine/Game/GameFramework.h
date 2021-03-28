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

		map<string, MetaClass*>& GetMetaClassesTable() { return MetaClassesTable; }

	private:

		Camera camera;
		World world;

		map<string, MetaClass*> MetaClassesTable;
};