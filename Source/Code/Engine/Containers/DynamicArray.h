#pragma once

#include "DefaultAllocator.h"

template<typename T, typename Allocator = DefaultAllocator>
class DynamicArray
{
	public:

		DynamicArray()
		{
			ArrayData = nullptr;

			ArrayLength = 0;
			ArrayCapacity = 0;
		}

		DynamicArray(const DynamicArray& OtherArray)
		{
			ArrayLength = OtherArray.ArrayLength;
			ArrayCapacity = OtherArray.ArrayCapacity;

			ArrayData = (T*)Allocator::AllocateMemory(sizeof(T) * ArrayCapacity);

			for (int i = 0; i < ArrayLength; i++)
			{
				ArrayData[i] = OtherArray.ArrayData[i];
			}
		}

		DynamicArray& operator=(const DynamicArray& OtherArray)
		{
			for (int i = 0; i < ArrayLength; i++)
			{
				ArrayData[i].~T();
			}

			Allocator::FreeMemory(ArrayData);

			ArrayLength = OtherArray.ArrayLength;
			ArrayCapacity = OtherArray.ArrayCapacity;

			ArrayData = (T*)Allocator::AllocateMemory(sizeof(T) * ArrayCapacity);

			for (int i = 0; i < ArrayLength; i++)
			{
				ArrayData[i] = OtherArray.ArrayData[i];
			}

			return *this;
		}

		~DynamicArray()
		{
			for (int i = 0; i < ArrayLength; i++)
			{
				ArrayData[i].~T();
			}

			Allocator::FreeMemory(ArrayData);
		}

		void Add(const T& Element)
		{
			if (ArrayCapacity == 0 || ArrayLength == ArrayCapacity)
			{
				T *OldArrayData = ArrayData;

				ArrayCapacity++;

				ArrayData = (T*)Allocator::AllocateMemory(sizeof(T) * ArrayCapacity);

				for (size_t i = 0; i < ArrayLength; i++)
				{
					ArrayData[i] = OldArrayData[i];
					OldArrayData[i].~T();
				}

				ArrayData[ArrayLength] = Element;

				Allocator::FreeMemory(OldArrayData);

				ArrayLength++;

			}
			else
			{
				ArrayData[ArrayLength] = Element;

				ArrayLength++;
			}
		}

		void Remove(size_t Index)
		{
			ArrayData[Index].~T();

			for (size_t i = Index; i < ArrayLength - 1; i++)
			{
				ArrayData[i] = ArrayData[i + 1];
			}
		}

		void Append(DynamicArray<T>& OtherArray)
		{
			T *OldArrayData = ArrayData;

			ArrayCapacity += OtherArray.ArrayCapacity;

			ArrayData = (T*)Allocator::AllocateMemory(sizeof(T) * ArrayCapacity);

			for (int i = 0; i < ArrayLength; i++)
			{
				ArrayData[i] = OldArrayData[i];
				OldArrayData[i].~T();
			}

			for (int i = 0; i < OtherArray.ArrayLength; i++)
			{
				ArrayData[ArrayLength + i] = OtherArray.ArrayData[i];
			}

			ArrayLength += OtherArray.ArrayLength;

			Allocator::FreeMemory(OldArrayData);
		}

		T& operator[](size_t Index)
		{
			return ArrayData[Index];
		}

		const T& operator[](size_t Index) const
		{
			return ArrayData[Index];
		}

		const size_t GetLength() const
		{
			return ArrayLength;
		}

		const T* GetData() const
		{
			return ArrayData;
		}

		void Clear()
		{
			for (int i = 0; i < ArrayLength; i++)
			{
				ArrayData[i].~T();
			}

			ArrayLength = 0;
		}

	private:

		T *ArrayData;

		size_t ArrayLength;
		size_t ArrayCapacity;
};