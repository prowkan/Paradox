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

		template<typename T>
		T* SpawnEntity();

		vector<Entity*>& GetEntities() { return Entities; }

		RenderScene& GetRenderScene() { return renderScene; }

	private:

		vector<Entity*> Entities;

		RenderScene renderScene;
};

template<typename T>
inline T* World::SpawnEntity()
{
	return Entity::DynamicCast<T>(SpawnEntity(T::GetMetaClassStatic()));
}