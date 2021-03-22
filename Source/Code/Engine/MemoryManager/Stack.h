#pragma once

#include "ScopedMemoryBlock.h"

class Stack
{
	public:

		void CreateStack(const size_t StackSize);
		void DestroyStack();

		template<typename T>
		ScopedMemoryBlock<T> AllocateFromStack();

		template<typename T>
		ScopedMemoryBlockArray<T> AllocateFromStack(const size_t ElementsCount);

		void DeAllocateToStack(const size_t BlockSize)
		{
			StackOffset -= BlockSize;
		}

	private:

		void *StackData = nullptr;
		size_t StackOffset;
};

template<typename T>
inline ScopedMemoryBlock<T> Stack::AllocateFromStack()
{
	T* BlockData = (T*)((BYTE*)StackData + StackOffset);
	size_t BlockSize = sizeof(T);
	StackOffset += BlockSize;

	return ScopedMemoryBlock<T>(BlockData, BlockSize);
}

template<typename T>
inline ScopedMemoryBlockArray<T> Stack::AllocateFromStack(const size_t ElementsCount)
{
	T* BlockData = (T*)((BYTE*)StackData + StackOffset);
	size_t BlockSize = sizeof(T) * ElementsCount;
	StackOffset += BlockSize;

	return ScopedMemoryBlockArray<T>(BlockData, BlockSize, ElementsCount);
}