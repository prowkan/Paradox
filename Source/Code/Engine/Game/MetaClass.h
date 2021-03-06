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

		bool IsBaseOf(MetaClass* metaClass) const
		{
			if (metaClass == this) return true;

			if (this->BaseClass && this->BaseClass == metaClass) return true;

			if (this->BaseClass && this->BaseClass->IsBaseOf(metaClass)) return true;

			return false;
		}

		uint32_t InstancesCount = 0;

	private:

		size_t ClassSize;

		const char *ClassName;

		MetaClass *BaseClass;
};

#define DECLARE_CLASS(Class) \
public: \
\
static void StaticConstructor(void* Pointer) { new (Pointer) Class(); } \
\
static MetaClass* GetMetaClassStatic() { return Class ## StaticMetaClass; }\
\
static void InitMetaClass() { Class ## StaticMetaClass = new MetaClass(&CallObjectConstructor<Class>, sizeof(Class), # Class); }\
\
private: \
\
static MetaClass *Class ## StaticMetaClass;\

#define DECLARE_CLASS_WITH_BASE_CLASS(Class, BaseClass) \
public: \
\
static void StaticConstructor(void* Pointer) { new (Pointer) Class(); } \
\
static MetaClass* GetMetaClassStatic() { return Class ## StaticMetaClass; } \
\
static void InitMetaClass() { Class ## StaticMetaClass = new MetaClass(&CallObjectConstructor<Class>, sizeof(Class), # Class, BaseClass::GetMetaClassStatic()); } \
\
private: \
\
static MetaClass *Class ## StaticMetaClass;\

#define DEFINE_METACLASS_VARIABLE(Class) MetaClass *Class::Class ## StaticMetaClass;