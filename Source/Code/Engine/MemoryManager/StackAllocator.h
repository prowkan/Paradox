#pragma once

class StackAllocator
{
	public:

		void CreateStackAllocator(const size_t StackSize);
		void DestroyStackAllocator();

		void* AllocateFromStack(const size_t BlockSize)
		{
			void* Pointer = (BYTE*)StackAllocatorData + StackAllocatorOffset;
			StackAllocatorOffset += BlockSize;
			return Pointer;
		}

		void DeAllocateToStack(const size_t BlockSize)
		{
			StackAllocatorOffset -= BlockSize;
		}

	private:

		void *StackAllocatorData = nullptr;
		size_t StackAllocatorOffset;
};