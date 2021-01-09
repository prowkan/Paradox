#include "Heap.h"

void Heap::CreateHeap(const size_t HeapSize)
{
	HeapData = HeapAlloc(GetProcessHeap(), 0, HeapSize);
}

void Heap::DestroyHeap()
{
	BOOL Result = HeapFree(GetProcessHeap(), 0, HeapData);
}

void* Heap::AllocateMemory(const size_t Size)
{
	void *Pointer = (BYTE*)HeapData + HeapOffset;
	HeapOffset += Size;
	return Pointer;
}