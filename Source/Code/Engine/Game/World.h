#pragma once

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

		vector<Entity*>& GetEntities() { return Entities; }

		RenderScene& GetRenderScene() { return renderScene; }

	private:

		vector<Entity*> Entities;

		RenderScene renderScene;
};