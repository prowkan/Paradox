#include "GameFramework.h"

#include "MetaClass.h"

#include "GameObject.h"
#include "GameObjects/Render/Meshes/StaticMeshObject.h"

#include "Component.h"
#include "Components/Common/TransformComponent.h"
#include "Components/Common/BoundingBoxComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"

void GameFramework::InitFramework()
{
	GameObjectMetaClass = new MetaClass(&CallObjectConstructor<GameObject>, sizeof(GameObject), "GameObject");
	StaticMeshObjectMetaClass = new MetaClass(&CallObjectConstructor<StaticMeshObject>, sizeof(StaticMeshObject), "StaticMeshObject", GameObjectMetaClass);

	ComponentMetaClass = new MetaClass(&CallObjectConstructor<Component>, sizeof(Component), "Component");
	TransformComponentMetaClass = new MetaClass(&CallObjectConstructor<TransformComponent>, sizeof(TransformComponent), "TransformComponent", ComponentMetaClass);
	BoundingBoxComponentMetaClass = new MetaClass(&CallObjectConstructor<BoundingBoxComponent>, sizeof(BoundingBoxComponent), "BoundingBoxComponent", ComponentMetaClass);
	StaticMeshComponentMetaClass = new MetaClass(&CallObjectConstructor<StaticMeshComponent>, sizeof(StaticMeshComponent), "StaticMeshComponent", ComponentMetaClass);

	camera.InitCamera();
	world.LoadWorld();
}

void GameFramework::ShutdownFramework()
{
	camera.ShutdownCamera();
	world.UnLoadWorld();
}

void GameFramework::TickFramework(float DeltaTime)
{
	camera.TickCamera(DeltaTime);
	world.TickWorld(DeltaTime);
}