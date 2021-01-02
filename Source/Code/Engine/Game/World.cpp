#include "World.h"

#include "MetaClass.h"
#include "GameObject.h"

#include "GameObjects/Render/Meshes/StaticMeshObject.h"

#include "Components/Common/TransformComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"

#include <Engine/Engine.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>

void World::LoadWorld()
{
	for (int i = -50; i < 50; i++)
	{
		for (int j = -50; j < 50; j++)
		{
			StaticMeshObject *staticMeshObject = (StaticMeshObject*)SpawnGameObject(StaticMeshObject::GetMetaClassStatic());
			staticMeshObject->GetTransformComponent()->SetLocation(XMFLOAT3(i * 5.0f + 2.5f, -0.0f, j * 5.0f + 2.5f));

			staticMeshObject = (StaticMeshObject*)SpawnGameObject(StaticMeshObject::GetMetaClassStatic());
			staticMeshObject->GetTransformComponent()->SetLocation(XMFLOAT3(i * 10.0f + 5.0f, -2.0f, j * 10.0f + 5.0f));
			staticMeshObject->GetTransformComponent()->SetScale(XMFLOAT3(5.0f, 1.0f, 5.0f));
		}
	}
}

void World::UnLoadWorld()
{

}

GameObject* World::SpawnGameObject(MetaClass* metaClass)
{
	void *gameObjectPtr = malloc(metaClass->GetClassSize());
	metaClass->ObjectConstructorFunc(gameObjectPtr);
	GameObject *gameObject = (GameObject*)gameObjectPtr;
	gameObject->InitDefaultProperties();
	GameObjects.push_back(gameObject);
	return gameObject;
}