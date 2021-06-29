#pragma once

template<typename T>
class StaticReference
{
	public:

		void CreateInstance()
		{
			new (DataStorage) T();
		}

		void DestroyInstance()
		{
			((T*)DataStorage)->~T();
		}

		T& operator*()
		{
			return *(T*)DataStorage;
		}

		operator T&()
		{
			return *(T*)DataStorage;
		}

		T* operator->()
		{
			return (T*)DataStorage;
		}

		T* operator&()
		{
			return (T*)DataStorage;
		}

	private:

		BYTE DataStorage[sizeof(T)];
};