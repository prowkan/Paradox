#pragma once

#include <Render/RenderScene.h>

class MetaClass;
class GameObject;

class World
{
	public:

		void LoadWorld();
		void UnLoadWorld();
		void TickWorld(float DeltaTime);

		GameObject* SpawnGameObject(MetaClass* metaClass);

		vector<GameObject*>& GetGameObjects() { return GameObjects; }

		RenderScene& GetRenderScene() { return renderScene; }

	private:

		vector<GameObject*> GameObjects;

		RenderScene renderScene;
};