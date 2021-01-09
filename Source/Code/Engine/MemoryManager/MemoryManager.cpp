#include "MemoryManager.h"

#include <Game/MetaClass.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

void MemoryManager::InitManager()
{
	TransformComponentsPool.CreatePool(sizeof(TransformComponent), 20000);
	BoundingBoxComponentsPool.CreatePool(sizeof(BoundingBoxComponent), 20000);
	StaticMeshComponentsPool.CreatePool(sizeof(StaticMeshComponent), 20000);
}

void MemoryManager::ShutdownManager()
{
	TransformComponentsPool.DestroyPool();
	BoundingBoxComponentsPool.DestroyPool();
	StaticMeshComponentsPool.DestroyPool();
}

void* MemoryManager::AllocateComponent(MetaClass* metaClass)
{
	if (metaClass == TransformComponent::GetMetaClassStatic()) return TransformComponentsPool.AllocateObject();
	if (metaClass == BoundingBoxComponent::GetMetaClassStatic()) return BoundingBoxComponentsPool.AllocateObject();
	if (metaClass == StaticMeshComponent::GetMetaClassStatic()) return StaticMeshComponentsPool.AllocateObject();

	return malloc(metaClass->GetClassSize());
}