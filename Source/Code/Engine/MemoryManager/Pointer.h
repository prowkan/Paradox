#pragma once

#include "SystemMemoryAllocator.h"

template<typename T>
class Pointer
{
	public:

		static Pointer<T> Create()
		{
			Pointer<T> pointer;

			pointer.Data = (T*)SystemMemoryAllocator::AllocateMemory(sizeof(T));

			return pointer;
		}

		Pointer()
		{
			Data = nullptr;
		}

		Pointer(const Pointer<T>& OtherPointer)
		{
			Data = OtherPointer.Data;
			OtherPointer.Data = nullptr;
		}

		~Pointer()
		{
			SystemMemoryAllocator::FreeMemory(Data);
		}

		template<typename U>
		operator U*()
		{
			return (U*)Data;
		}

	private:

		T *Data;
};

template<typename T>
class Pointer<T[]>
{
	public:

		static Pointer<T[]> Create(const size_t ElementsCount)
		{
			Pointer<T[]> pointer;

			pointer.Data = (T*)SystemMemoryAllocator::AllocateMemory(sizeof(T) * ElementsCount);

			return pointer;
		}

		Pointer()
		{
			Data = nullptr;
		}

		Pointer(const Pointer<T[]>& OtherPointer)
		{
			Data = OtherPointer.Data;
			((Pointer<T[]>&)OtherPointer).Data = nullptr;
		}

		~Pointer()
		{
			SystemMemoryAllocator::FreeMemory(Data);
		}

		template<typename U>
		operator U*()
		{
			return (U*)this->Data;
		}

		T& operator[](const size_t Index)
		{
			return this->Data[Index];
		}		

	private:

		T *Data;
};