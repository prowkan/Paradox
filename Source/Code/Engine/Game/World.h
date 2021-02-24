#pragma once

#include <Containers/DynamicArray.h>

#include <Render/RenderScene.h>

class MetaClass;
class Entity;

class World
{
	public:

		void LoadWorld();
		void UnLoadWorld();
		void TickWorld(float DeltaTime);

		Entity* SpawnEntity(MetaClass* metaClass);

		DynamicArray<Entity*>& GetEntities() { return Entities; }

		RenderScene& GetRenderScene() { return renderScene; }

	private:

		DynamicArray<Entity*> Entities;

		RenderScene renderScene;
};