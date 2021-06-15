#pragma once

#include "ScopedMemoryBlock.h"

class StackAllocator
{
	public:

		void CreateStackAllocator(const size_t StackSize);
		void DestroyStackAllocator();

		template<typename T>
		ScopedMemoryBlock<T> AllocateFromStack();

		template<typename T>
		ScopedMemoryBlockArray<T> AllocateFromStack(const size_t ElementsCount);

		void DeAllocateToStack(const size_t BlockSize)
		{
			StackAllocatorOffset -= BlockSize;
		}

	private:

		void *StackAllocatorData = nullptr;
		size_t StackAllocatorOffset;
};

template<typename T>
inline ScopedMemoryBlock<T> StackAllocator::AllocateFromStack()
{
	T* BlockData = (T*)((BYTE*)StackAllocatorData + StackAllocatorOffset);
	size_t BlockSize = sizeof(T);
	StackAllocatorOffset += BlockSize;

	return ScopedMemoryBlock<T>(BlockData, BlockSize);
}

template<typename T>
inline ScopedMemoryBlockArray<T> StackAllocator::AllocateFromStack(const size_t ElementsCount)
{
	T* BlockData = (T*)((BYTE*)StackAllocatorData + StackAllocatorOffset);
	size_t BlockSize = sizeof(T) * ElementsCount;
	StackAllocatorOffset += BlockSize;

	return ScopedMemoryBlockArray<T>(BlockData, BlockSize, ElementsCount);
}