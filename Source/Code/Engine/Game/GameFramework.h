#pragma once

#include "Camera.h"
#include "World.h"

#include <Containers/HashTable.h>

class GameFramework
{
	public:

		void InitFramework();
		void ShutdownFramework();
		void TickFramework(float DeltaTime);

		Camera& GetCamera() { return camera; }
		World& GetWorld() { return world; }

		HashTable<String, MetaClass*>& GetMetaClassesTable() { return MetaClassesTable; }

	private:

		Camera camera;
		World world;

		HashTable<String, MetaClass*> MetaClassesTable;
};