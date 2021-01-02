#pragma once

class MetaClass;
class GameObject;

class World
{
	public:

		void LoadWorld();
		void UnLoadWorld();

		GameObject* SpawnGameObject(MetaClass* metaClass);

	private:

		vector<GameObject*> GameObjects;
};