#pragma once

#include <MemoryManager/SystemAllocator.h>

class DefaultAllocator
{
	public:

		static void* AllocateMemory(const size_t Size) { return SystemAllocator::AllocateMemory(Size); }
		static void FreeMemory(void* Pointer) { SystemAllocator::FreeMemory(Pointer); }
};