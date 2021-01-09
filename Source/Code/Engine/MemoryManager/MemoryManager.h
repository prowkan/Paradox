#pragma once

#include "Pool.h"

class MetaClass;

class MemoryManager
{
	public:

		void InitManager();
		void ShutdownManager();

		void* AllocateComponent(MetaClass* metaClass);

	private:

		Pool TransformComponentsPool;
		Pool BoundingBoxComponentsPool;
		Pool StaticMeshComponentsPool;
};