// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Pool.h"

#include "SystemMemoryAllocator.h"

void Pool::CreatePool(const size_t ObjectSize, const size_t MaxObjectsInPool)
{
	PoolData = SystemMemoryAllocator::AllocateMemory(ObjectSize * MaxObjectsInPool);

	this->ObjectSize = ObjectSize;
	this->ObjectsCount = 0;
}

void Pool::DestroyPool()
{
	SystemMemoryAllocator::FreeMemory(PoolData);
	PoolData = nullptr;
}

void* Pool::AllocateObject()
{
	void *Pointer = (BYTE*)PoolData + ObjectSize * ObjectsCount;

	++ObjectsCount;

	return Pointer;
}