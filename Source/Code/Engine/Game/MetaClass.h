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

		MetaClass(ObjectConstructorType ObjectConstructorFunc, const size_t ClassSize, const char* ClassName) : ObjectConstructorFunc(ObjectConstructorFunc), ClassSize(ClassSize), ClassName(ClassName) {}

		ObjectConstructorType ObjectConstructorFunc;

		const size_t GetClassSize() const { return ClassSize; }

		const char* GetClassName() const { return ClassName; }

	private:

		size_t ClassSize;

		const char *ClassName;
};