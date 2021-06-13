// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MemoryManager.h"

#include "SystemAllocator.h"

#include <Game/MetaClass.h>

#include <Game/Entities/Render/Meshes/StaticMeshEntity.h>
#include <Game/Entities/Render/Lights/PointLightEntity.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

void MemoryManager::InitManager()
{
	EntitiesHeap.CreateHeap(20000 * sizeof(StaticMeshEntity) + 10000 * sizeof(PointLightEntity));

	TransformComponentsPool.CreatePool(sizeof(TransformComponent), 30000);
	BoundingBoxComponentsPool.CreatePool(sizeof(BoundingBoxComponent), 20000);
	StaticMeshComponentsPool.CreatePool(sizeof(StaticMeshComponent), 20000);
	PointLightComponentsPool.CreatePool(sizeof(PointLightComponent), 10000);

	GlobalStack.CreateStack(20 * 1024 * 1024);
}

void MemoryManager::ShutdownManager()
{
	TransformComponentsPool.DestroyPool();
	BoundingBoxComponentsPool.DestroyPool();
	StaticMeshComponentsPool.DestroyPool();
	PointLightComponentsPool.DestroyPool();

	EntitiesHeap.DestroyHeap();

	GlobalStack.DestroyStack();
}

void* MemoryManager::AllocateEntity(MetaClass* metaClass)
{
	return EntitiesHeap.AllocateMemory(metaClass->GetClassSize());
}

void* MemoryManager::AllocateComponent(MetaClass* metaClass)
{
	if (metaClass == TransformComponent::GetMetaClassStatic()) return TransformComponentsPool.AllocateObject();
	if (metaClass == BoundingBoxComponent::GetMetaClassStatic()) return BoundingBoxComponentsPool.AllocateObject();
	if (metaClass == StaticMeshComponent::GetMetaClassStatic()) return StaticMeshComponentsPool.AllocateObject();
	if (metaClass == PointLightComponent::GetMetaClassStatic()) return PointLightComponentsPool.AllocateObject();

	return malloc(metaClass->GetClassSize());
}