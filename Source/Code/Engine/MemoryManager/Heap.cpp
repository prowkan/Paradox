// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Heap.h"

#include "SystemAllocator.h"

void Heap::CreateHeap(const size_t HeapSize)
{
	HeapData = SystemAllocator::AllocateMemory(HeapSize);
	HeapOffset = 0;
}

void Heap::DestroyHeap()
{
	SystemAllocator::FreeMemory(HeapData);
	HeapData = nullptr;
	HeapOffset = 0;
}

void* Heap::AllocateMemory(const size_t Size)
{
	void *Pointer = (BYTE*)HeapData + HeapOffset;
	HeapOffset += Size;
	return Pointer;
}