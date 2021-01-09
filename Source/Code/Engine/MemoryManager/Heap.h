#pragma once

class Heap
{
	public:

		void CreateHeap(const size_t HeapSize);
		void DestroyHeap();

		void* AllocateMemory(const size_t Size);

	private:

		void *HeapData;
		size_t HeapOffset;
};