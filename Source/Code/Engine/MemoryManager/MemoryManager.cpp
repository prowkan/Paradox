#include "MemoryManager.h"

#include <Game/MetaClass.h>

#include <Game/GameObjects/Render/Meshes/StaticMeshObject.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

void MemoryManager::InitManager()
{
	GameObjectsHeap.CreateHeap(20000 * sizeof(StaticMeshObject));

	TransformComponentsPool.CreatePool(sizeof(TransformComponent), 20000);
	BoundingBoxComponentsPool.CreatePool(sizeof(BoundingBoxComponent), 20000);
	StaticMeshComponentsPool.CreatePool(sizeof(StaticMeshComponent), 20000);
}

void MemoryManager::ShutdownManager()
{
	TransformComponentsPool.DestroyPool();
	BoundingBoxComponentsPool.DestroyPool();
	StaticMeshComponentsPool.DestroyPool();

	GameObjectsHeap.DestroyHeap();
}

void* MemoryManager::AllocateGameObject(MetaClass* metaClass)
{
	return GameObjectsHeap.AllocateMemory(metaClass->GetClassSize());
}

void* MemoryManager::AllocateComponent(MetaClass* metaClass)
{
	if (metaClass == TransformComponent::GetMetaClassStatic()) return TransformComponentsPool.AllocateObject();
	if (metaClass == BoundingBoxComponent::GetMetaClassStatic()) return BoundingBoxComponentsPool.AllocateObject();
	if (metaClass == StaticMeshComponent::GetMetaClassStatic()) return StaticMeshComponentsPool.AllocateObject();

	return malloc(metaClass->GetClassSize());
}