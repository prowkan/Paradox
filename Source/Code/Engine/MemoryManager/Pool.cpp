// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Pool.h"

#include "SystemAllocator.h"

void Pool::CreatePool(const size_t ObjectSize, const size_t MaxObjectsInPool)
{
	PoolData = SystemAllocator::AllocateMemory(ObjectSize * MaxObjectsInPool);

	this->ObjectSize = ObjectSize;
	this->ObjectsCount = 0;
}

void Pool::DestroyPool()
{
	SystemAllocator::FreeMemory(PoolData);
	PoolData = nullptr;
}

void* Pool::AllocateObject()
{
	void *Pointer = (BYTE*)PoolData + ObjectSize * ObjectsCount;

	++ObjectsCount;

	return Pointer;
}