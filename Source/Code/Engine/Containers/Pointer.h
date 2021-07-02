#pragma once

#include <MemoryManager/SystemMemoryAllocator.h>

template<typename T>
class Pointer
{
	public:

		template<typename ...ArgTypes>
		static Pointer<T> Create(ArgTypes... Args)
		{
			Pointer<T> pointer;

			pointer.Data = (T*)SystemMemoryAllocator::AllocateMemory(sizeof(T));
			new (pointer.Data) T(Args...);

			return pointer;
		}

		Pointer()
		{
			Data = nullptr;
		}

		Pointer(const Pointer<T>& OtherPointer) = delete;
		Pointer<T>& operator=(const Pointer<T>& OtherPointer) = delete;

		Pointer(Pointer<T>&& OtherPointer)
		{
			Data = OtherPointer.Data;
			OtherPointer.Data = nullptr;
		}

		Pointer<T>& operator=(Pointer<T>&& OtherPointer)
		{
			if (Data != nullptr) Data->~T();
			Data = OtherPointer.Data;
			OtherPointer.Data = nullptr;

			return *this;
		}

		~Pointer()
		{
			if (Data != nullptr) Data->~T();
			SystemMemoryAllocator::FreeMemory(Data);
		}

		T* operator->()
		{
			return Data;
		}

		T& operator*()
		{
			return *Data;
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

		template<typename ...ArgTypes>
		static Pointer<T[]> Create(const size_t ElementsCount, ArgTypes... Args)
		{
			Pointer<T[]> pointer;

			pointer.Data = (T*)SystemMemoryAllocator::AllocateMemory(sizeof(T) * ElementsCount);
			for (size_t i = 0; i < ElementsCount; i++)
			{
				new (&pointer.Data[i]) T(Args...);
			}
			pointer.ElementsCount = ElementsCount;

			return pointer;
		}

		Pointer()
		{
			Data = nullptr;
		}

		Pointer(const Pointer<T[]>& OtherPointer)
		{
			Data = (T*)SystemMemoryAllocator::AllocateMemory(sizeof(T) * OtherPointer.ElementsCount);
			ElementsCount = OtherPointer.ElementsCount;
			for (size_t i = 0; i < ElementsCount; i++)
			{
				new (&Data[i]) T(OtherPointer.Data[i]);
			}
		}

		~Pointer()
		{
			for (size_t i = 0; i < ElementsCount; i++)
			{
				Data[i].~T();
			}
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
		size_t ElementsCount;
};