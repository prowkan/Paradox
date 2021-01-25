// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MemoryManager.h"

#include <Game/MetaClass.h>

#include <Game/Entities/Render/Meshes/StaticMeshEntity.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

void MemoryManager::InitManager()
{
	EntitiesHeap.CreateHeap(20000 * sizeof(StaticMeshEntity));

	TransformComponentsPool.CreatePool(sizeof(TransformComponent), 20000);
	BoundingBoxComponentsPool.CreatePool(sizeof(BoundingBoxComponent), 20000);
	StaticMeshComponentsPool.CreatePool(sizeof(StaticMeshComponent), 20000);
}

void MemoryManager::ShutdownManager()
{
	TransformComponentsPool.DestroyPool();
	BoundingBoxComponentsPool.DestroyPool();
	StaticMeshComponentsPool.DestroyPool();

	EntitiesHeap.DestroyHeap();
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

	return malloc(metaClass->GetClassSize());
}