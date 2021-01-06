#include "GameFramework.h"

#include "MetaClass.h"

#include "GameObject.h"
#include "GameObjects/Render/Meshes/StaticMeshObject.h"

#include "Component.h"
#include "Components/Common/TransformComponent.h"
#include "Components/Common/BoundingBoxComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"

#define DECLARE_CLASS(Class) Class::Class ## MetaClass = new MetaClass(&CallObjectConstructor<Class>, sizeof(Class), # Class);
#define DECLARE_CLASS_WITH_BASE_CLASS(Class, BaseClass) Class::Class ## MetaClass = new MetaClass(&CallObjectConstructor<Class>, sizeof(Class), # Class, BaseClass::BaseClass ## MetaClass);

void GameFramework::InitFramework()
{
	DECLARE_CLASS(GameObject)
	DECLARE_CLASS_WITH_BASE_CLASS(StaticMeshObject, GameObject)

	DECLARE_CLASS(Component)
	DECLARE_CLASS_WITH_BASE_CLASS(TransformComponent, GameObject)
	DECLARE_CLASS_WITH_BASE_CLASS(BoundingBoxComponent, GameObject)
	DECLARE_CLASS_WITH_BASE_CLASS(StaticMeshComponent, GameObject)

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