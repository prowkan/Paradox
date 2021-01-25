// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GameFramework.h"

#include "MetaClass.h"

#include "Entity.h"
#include "Entities/Render/Meshes/StaticMeshEntity.h"

#include "Component.h"
#include "Components/Common/TransformComponent.h"
#include "Components/Common/BoundingBoxComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"

void GameFramework::InitFramework()
{
	Entity::InitMetaClass();
	StaticMeshEntity::InitMetaClass();

	Component::InitMetaClass();
	TransformComponent::InitMetaClass();
	BoundingBoxComponent::InitMetaClass();
	StaticMeshComponent::InitMetaClass();

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