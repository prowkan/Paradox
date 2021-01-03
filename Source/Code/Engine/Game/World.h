#pragma once

class MetaClass;
class GameObject;

class World
{
	public:

		void LoadWorld();
		void UnLoadWorld();

		GameObject* SpawnGameObject(MetaClass* metaClass);

		vector<GameObject*>& GetGameObjects() { return GameObjects; }

	private:

		vector<GameObject*> GameObjects;
};