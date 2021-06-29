// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PCWindowsPlatformMemoryAllocator.h"

HANDLE PCWindowsPlatformMemoryAllocator::ProcessHeap;

void PCWindowsPlatformMemoryAllocator::InitAllocator()
{
	ProcessHeap = GetProcessHeap();
}

void* PCWindowsPlatformMemoryAllocator::AllocateMemory(const size_t Size)
{
	return HeapAlloc(ProcessHeap, 0, Size);
}

void PCWindowsPlatformMemoryAllocator::FreeMemory(void *Pointer)
{
	BOOL Result = HeapFree(ProcessHeap, 0, Pointer);
}