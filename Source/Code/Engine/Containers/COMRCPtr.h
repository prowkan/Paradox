#pragma once

template<typename T>
class COMRCPtr
{
	public:

		COMRCPtr(T *Pointer = nullptr) : Pointer(Pointer) {};

		COMRCPtr(const COMRCPtr& Other)
		{
			ULONG RefCount = Other.Pointer->AddRef();
			Pointer = Other.Pointer;
		}

		COMRCPtr(COMRCPtr&& Other)
		{
			Pointer = Other.Pointer;
			Other.Pointer = nullptr;
		}

		COMRCPtr& operator=(const COMRCPtr& Other)
		{
			ULONG RefCount = Other.Pointer->AddRef();
			Pointer = Other.Pointer;
			return *this;
		}

		COMRCPtr& operator=(COMRCPtr&& Other)
		{
			Pointer = Other.Pointer;
			Other.Pointer = nullptr;
			return *this;
		}

		~COMRCPtr() { if (Pointer) { ULONG RefCount = Pointer->Release(); Pointer = nullptr; } }

		T** operator&() { return &Pointer; }
		T* operator->() { return Pointer; }
		operator T*() { return Pointer; }

	private:

		T *Pointer;
};