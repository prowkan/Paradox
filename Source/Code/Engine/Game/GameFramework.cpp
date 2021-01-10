#include "GameFramework.h"

#include "MetaClass.h"

#include "Entity.h"
#include "Entities/Render/Meshes/StaticMeshEntity.h"

#include "Component.h"
#include "Components/Common/TransformComponent.h"
#include "Components/Common/BoundingBoxComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"

#define DECLARE_CLASS(Class) Class::Class ## MetaClass = new MetaClass(&CallObjectConstructor<Class>, sizeof(Class), # Class);
#define DECLARE_CLASS_WITH_BASE_CLASS(Class, BaseClass) Class::Class ## MetaClass = new MetaClass(&CallObjectConstructor<Class>, sizeof(Class), # Class, BaseClass::BaseClass ## MetaClass);

void GameFramework::InitFramework()
{
	DECLARE_CLASS(Entity)
	DECLARE_CLASS_WITH_BASE_CLASS(StaticMeshEntity, Entity)

	DECLARE_CLASS(Component)
	DECLARE_CLASS_WITH_BASE_CLASS(TransformComponent, Component)
	DECLARE_CLASS_WITH_BASE_CLASS(BoundingBoxComponent, Component)
	DECLARE_CLASS_WITH_BASE_CLASS(StaticMeshComponent, Component)

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