#pragma once

class SystemMemoryAllocator
{
	public:

		static void InitAllocator();

		static void* AllocateMemory(const size_t Size);
		static void FreeMemory(void *Pointer);

	private:
};