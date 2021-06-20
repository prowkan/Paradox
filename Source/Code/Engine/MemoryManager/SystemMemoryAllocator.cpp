// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SystemMemoryAllocator.h"

#include <Platform/PlatformMemoryAllocator.h>

void SystemMemoryAllocator::InitAllocator()
{
	PlatformMemoryAllocator::InitAllocator();
}

void* SystemMemoryAllocator::AllocateMemory(const size_t Size)
{
	return PlatformMemoryAllocator::AllocateMemory(Size);
}

void SystemMemoryAllocator::FreeMemory(void *Pointer)
{
	PlatformMemoryAllocator::FreeMemory(Pointer);
}