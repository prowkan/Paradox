#pragma once

//#include "ScopedMemoryBlock.h"

template<typename T>
class ScopedMemoryBlock;

template<typename T>
class ScopedMemoryBlockArray;

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
class ScopedMemoryBlock
{
	public:

		ScopedMemoryBlock(Stack& StackAllocator, T* BlockData, const size_t BlockSize) : StackAllocator(StackAllocator), BlockData(BlockData), BlockSize(BlockSize)
		{

		}

		~ScopedMemoryBlock()
		{
			StackAllocator.DeAllocateToStack(BlockSize);
		}

		operator void*()
		{
			return BlockData;
		}

		operator T*()
		{
			return BlockData;
		}

		template<typename U>
		operator U*()
		{
			return (U*)BlockData;
		}

	protected:

		T *BlockData;
		size_t BlockSize;

	private:

		Stack& StackAllocator;
};

template<typename T>
class ScopedMemoryBlockArray : public ScopedMemoryBlock<T>
{
	public:

		ScopedMemoryBlockArray(Stack& StackAllocator, T* BlockData, const size_t BlockSize, const size_t ElementsCount) : ScopedMemoryBlock<T>(StackAllocator, BlockData, BlockSize), ElementsCount(ElementsCount)
		{

		}

		operator void*()
		{
			return ScopedMemoryBlock<T>::BlockData;
		}

		operator T*()
		{
			return ScopedMemoryBlock<T>::BlockData;
		}

		template<typename U>
		operator U*()
		{
			return (U*)ScopedMemoryBlock<T>::BlockData;
		}

	private:

		size_t ElementsCount;
};

template<typename T>
inline ScopedMemoryBlock<T> Stack::AllocateFromStack()
{
	T* BlockData = (T*)((BYTE*)StackData + StackOffset);
	size_t BlockSize = sizeof(T);
	StackOffset += BlockSize;

	return ScopedMemoryBlock<T>(*this, BlockData, BlockSize);
}

template<typename T>
inline ScopedMemoryBlockArray<T> Stack::AllocateFromStack(const size_t ElementsCount)
{
	T* BlockData = (T*)((BYTE*)StackData + StackOffset);
	size_t BlockSize = sizeof(T) * ElementsCount;
	StackOffset += BlockSize;

	return ScopedMemoryBlockArray<T>(*this, BlockData, BlockSize, ElementsCount);
}