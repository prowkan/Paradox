// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GameFramework.h"

#include "MetaClass.h"

#include "Entity.h"
#include "Entities/Render/Meshes/StaticMeshEntity.h"
#include "Entities/Render/Lights/PointLightEntity.h"

#include "Component.h"
#include "Components/Common/TransformComponent.h"
#include "Components/Common/BoundingBoxComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"
#include "Components/Render/Lights/PointLightComponent.h"

void GameFramework::InitFramework()
{
	Entity::InitMetaClass();
	StaticMeshEntity::InitMetaClass();
	PointLightEntity::InitMetaClass();

	Component::InitMetaClass();
	TransformComponent::InitMetaClass();
	BoundingBoxComponent::InitMetaClass();
	StaticMeshComponent::InitMetaClass();
	PointLightComponent::InitMetaClass();

	MetaClassesTable.Insert(Entity::GetMetaClassStatic()->GetClassName(), Entity::GetMetaClassStatic());
	MetaClassesTable.Insert(StaticMeshEntity::GetMetaClassStatic()->GetClassName(), StaticMeshEntity::GetMetaClassStatic());
	MetaClassesTable.Insert(PointLightEntity::GetMetaClassStatic()->GetClassName(), PointLightEntity::GetMetaClassStatic());

	MetaClassesTable.Insert(Component::GetMetaClassStatic()->GetClassName(), Component::GetMetaClassStatic());
	MetaClassesTable.Insert(TransformComponent::GetMetaClassStatic()->GetClassName(), TransformComponent::GetMetaClassStatic());
	MetaClassesTable.Insert(BoundingBoxComponent::GetMetaClassStatic()->GetClassName(), BoundingBoxComponent::GetMetaClassStatic());
	MetaClassesTable.Insert(StaticMeshComponent::GetMetaClassStatic()->GetClassName(), StaticMeshComponent::GetMetaClassStatic());
	MetaClassesTable.Insert(PointLightComponent::GetMetaClassStatic()->GetClassName(), PointLightComponent::GetMetaClassStatic());

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