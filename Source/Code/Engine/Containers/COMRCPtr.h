#pragma once

template<typename T>
class COMRCPtr
{
	public:

		COMRCPtr(T *Pointer = nullptr) : Pointer(Pointer) {};

		~COMRCPtr() { if (Pointer) { ULONG RefCount = Pointer->Release(); Pointer = nullptr; } }

		T** operator&() { return &Pointer; }
		T* operator->() { return Pointer; }
		operator T*() { return Pointer; }

	private:

		T *Pointer;
};