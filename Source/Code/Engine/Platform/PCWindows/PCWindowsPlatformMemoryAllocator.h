#pragma once

class PCWindowsPlatformMemoryAllocator
{
	public:

		static void InitAllocator();

		static void* AllocateMemory(const size_t Size);
		static void FreeMemory(void *Pointer);

	private:

		static HANDLE ProcessHeap;
};