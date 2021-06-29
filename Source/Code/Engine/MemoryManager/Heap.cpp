// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Heap.h"

#include "SystemMemoryAllocator.h"

void Heap::CreateHeap(const size_t HeapSize)
{
	HeapData = SystemMemoryAllocator::AllocateMemory(HeapSize);
	HeapOffset = 0;
}

void Heap::DestroyHeap()
{
	SystemMemoryAllocator::FreeMemory(HeapData);
	HeapData = nullptr;
	HeapOffset = 0;
}

void* Heap::AllocateMemory(const size_t Size)
{
	void *Pointer = (BYTE*)HeapData + HeapOffset;
	HeapOffset += Size;
	return Pointer;
}