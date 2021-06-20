#pragma once

#include <MemoryManager/SystemMemoryAllocator.h>

class DefaultAllocator
{
	public:

		static void* AllocateMemory(const size_t Size) { return SystemMemoryAllocator::AllocateMemory(Size); }
		static void FreeMemory(void* Pointer) { SystemMemoryAllocator::FreeMemory(Pointer); }
};