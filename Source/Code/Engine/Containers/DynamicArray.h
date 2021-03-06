#pragma once

template<typename T>
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

			ArrayData = (T*)malloc(sizeof(T) * ArrayCapacity);

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

			free(ArrayData);

			ArrayLength = OtherArray.ArrayLength;
			ArrayCapacity = OtherArray.ArrayCapacity;

			ArrayData = (T*)malloc(sizeof(T) * ArrayCapacity);

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

			free(ArrayData);
		}

		void Add(const T& Element)
		{
			if (ArrayCapacity == 0 || ArrayLength == ArrayCapacity)
			{
				T *OldArrayData = ArrayData;

				ArrayCapacity++;

				ArrayData = (T*)malloc(sizeof(T) * ArrayCapacity);

				for (size_t i = 0; i < ArrayLength; i++)
				{
					ArrayData[i] = OldArrayData[i];
					OldArrayData[i].~T();
				}

				ArrayData[ArrayLength] = Element;

				free(OldArrayData);

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

			ArrayData = (T*)malloc(sizeof(T) * ArrayCapacity);

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

			free(OldArrayData);
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

	private:

		T *ArrayData;

		size_t ArrayLength;
		size_t ArrayCapacity;
};