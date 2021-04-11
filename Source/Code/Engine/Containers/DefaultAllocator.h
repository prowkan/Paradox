#pragma once

class DefaultAllocator
{
	public:

		static void* AllocateMemory(const size_t Size) { return HeapAlloc(GetProcessHeap(), 0, Size); }
		static void FreeMemory(void* Pointer) { bool Result = HeapFree(GetProcessHeap(), 0, Pointer); }
};