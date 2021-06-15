#pragma once

#include "Pool.h"
#include "Heap.h"
#include "StackAllocator.h"

class MetaClass;

class MemoryManager
{
	public:

		void InitManager();
		void ShutdownManager();

		void* AllocateEntity(MetaClass* metaClass);
		void* AllocateComponent(MetaClass* metaClass);

		StackAllocator& GetGlobalStackAllocator() { return GlobalStackAllocator; }

	private:

		Pool TransformComponentsPool;
		Pool BoundingBoxComponentsPool;
		Pool StaticMeshComponentsPool;
		Pool PointLightComponentsPool;

		Heap EntitiesHeap;

		StackAllocator GlobalStackAllocator;
};