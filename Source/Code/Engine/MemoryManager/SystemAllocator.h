#pragma once

class SystemAllocator
{
	public:
		
		static void* AllocateMemory(const size_t Size);
		static void FreeMemory(void *Pointer);
};