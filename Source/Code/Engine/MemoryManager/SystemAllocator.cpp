// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SystemAllocator.h"

void* SystemAllocator::AllocateMemory(const size_t Size)
{
	static HANDLE ProcessHeap = GetProcessHeap();
	return HeapAlloc(ProcessHeap, 0, Size);
}

void SystemAllocator::FreeMemory(void *Pointer)
{
	static HANDLE ProcessHeap = GetProcessHeap();
	BOOL Result = HeapFree(ProcessHeap, 0, Pointer);
}