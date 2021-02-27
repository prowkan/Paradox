#pragma once

#include <Engine/Engine.h>

/*template<typename T>
class ScopedMemoryBlock
{
	public:

		ScopedMemoryBlock(T* BlockData, const size_t BlockSize) : BlockData(BlockData), BlockSize(BlockSize)
		{

		}

		~ScopedMemoryBlock()
		{
			Engine::GetEngine().GetMemoryManager().GetGlobalStack().DeAllocateToStack(BlockSize);
		}

		operator void*()
		{
			return BlockData;
		}

	protected:

		T *BlockData;
		size_t BlockSize;

	public:
};

template<typename T>
class ScopedMemoryBlockArray : public ScopedMemoryBlock<T>
{
	public:

		ScopedMemoryBlockArray(T* BlockData, const size_t BlockSize, const size_t ElementsCount) : ScopedMemoryBlock(BlockData, BlockSize), ElementsCount(ElementsCount)
		{

		}

		operator void*()
		{
			return BlockData;
		}

	private:		

		size_t ElementsCount;
};*/