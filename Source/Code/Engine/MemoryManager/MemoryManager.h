#pragma once

#include "Pool.h"
#include "Heap.h"
#include "Stack.h"

class MetaClass;

class MemoryManager
{
	public:

		void InitManager();
		void ShutdownManager();

		void* AllocateEntity(MetaClass* metaClass);
		void* AllocateComponent(MetaClass* metaClass);

		Stack& GetGlobalStack() { return GlobalStack; }

	private:

		Pool TransformComponentsPool;
		Pool BoundingBoxComponentsPool;
		Pool StaticMeshComponentsPool;
		Pool PointLightComponentsPool;

		Heap EntitiesHeap;

		Stack GlobalStack;
};