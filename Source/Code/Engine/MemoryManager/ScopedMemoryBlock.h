#pragma once

#include "ScopedMemoryBlockBase.h"

template<typename T>
class ScopedMemoryBlock : public ScopedMemoryBlockBase
{
	public:

		ScopedMemoryBlock(T* BlockData, const size_t BlockSize) : BlockData(BlockData) 
		{
			this->BlockSize = BlockSize;
		}

		template<typename U>
		operator U*()
		{
			return (U*)BlockData;
		}

	protected:

		T *BlockData;
};

template<typename T>
class ScopedMemoryBlockArray : public ScopedMemoryBlock<T>
{
	public:

		ScopedMemoryBlockArray(T* BlockData, const size_t BlockSize, const size_t ElementsCount) : ScopedMemoryBlock<T>(BlockData, BlockSize), ElementsCount(ElementsCount) {}

		template<typename U>
		operator U*()
		{
			return (U*)this->BlockData;
		}

		T& operator[](const size_t Index)
		{
			return this->BlockData[Index];
		}

	private:

		size_t ElementsCount;
};