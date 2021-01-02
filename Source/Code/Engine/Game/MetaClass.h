#pragma once

template<typename T>
void CallObjectConstructor(void* Pointer)
{
	T::StaticConstructor(Pointer);
}

using ObjectConstructorType = void(*)(void*);

class MetaClass
{
	public:

		MetaClass(ObjectConstructorType ObjectConstructorFunc, const size_t ClassSize) : ObjectConstructorFunc(ObjectConstructorFunc), ClassSize(ClassSize) {}

		ObjectConstructorType ObjectConstructorFunc;

		const size_t GetClassSize() const { return ClassSize; }

	private:

		size_t ClassSize;
};