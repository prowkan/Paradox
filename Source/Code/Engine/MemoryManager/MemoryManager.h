#pragma once

#include "Pool.h"
#include "Heap.h"

class MetaClass;

class MemoryManager
{
	public:

		void InitManager();
		void ShutdownManager();

		void* AllocateGameObject(MetaClass* metaClass);
		void* AllocateComponent(MetaClass* metaClass);

	private:

		Pool TransformComponentsPool;
		Pool BoundingBoxComponentsPool;
		Pool StaticMeshComponentsPool;

		Heap GameObjectsHeap;
};