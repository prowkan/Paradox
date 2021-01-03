#pragma once

#undef GetClassName

template<typename T>
void CallObjectConstructor(void* Pointer)
{
	T::StaticConstructor(Pointer);
}

using ObjectConstructorType = void(*)(void*);

class MetaClass
{
	public:

		MetaClass(ObjectConstructorType ObjectConstructorFunc, const size_t ClassSize, const char* ClassName, MetaClass* BaseClass = nullptr) : ObjectConstructorFunc(ObjectConstructorFunc), ClassSize(ClassSize), ClassName(ClassName), BaseClass(BaseClass) {}

		ObjectConstructorType ObjectConstructorFunc;

		const size_t GetClassSize() const { return ClassSize; }

		const char* GetClassName() const { return ClassName; }

		bool IsBaseOf(MetaClass *metaClass) const
		{
			if (metaClass == this) return true;

			if (this->BaseClass && this->BaseClass == metaClass) return true;

			if (this->BaseClass && this->BaseClass->IsBaseOf(metaClass)) return true;

			return false;
		}

	private:

		size_t ClassSize;

		const char *ClassName;

		MetaClass *BaseClass;
};