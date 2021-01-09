#include "Pool.h"

void Pool::CreatePool(const size_t ObjectSize, const size_t MaxObjectsInPool)
{
	PoolData = HeapAlloc(GetProcessHeap(), 0, ObjectSize * MaxObjectsInPool);

	this->ObjectSize = ObjectSize;
	this->ObjectsCount = 0;
}

void Pool::DestroyPool()
{
	BOOL Result = HeapFree(GetProcessHeap(), 0, PoolData);
}

void* Pool::AllocateObject()
{
	void *Pointer = (BYTE*)PoolData + ObjectSize * ObjectsCount;

	++ObjectsCount;

	return Pointer;
}