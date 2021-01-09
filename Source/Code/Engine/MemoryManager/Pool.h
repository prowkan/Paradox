#pragma once

class Pool
{
	public:

		void CreatePool(const size_t ObjectSize, const size_t MaxObjectsInPool);
		void DestroyPool();

		void *AllocateObject();

	private:

		void *PoolData;

		size_t ObjectSize;
		size_t ObjectsCount;
};