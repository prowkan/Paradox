#pragma once

#include "MetaClass.h"

class Component;
class Entity;

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
static MetaClass* GetMetaClassStatic() { return Class ## StaticMetaClass; }\
\
static void InitMetaClass() { Class ## StaticMetaClass = new MetaClass(&CallObjectConstructor<Class>, sizeof(Class), # Class, BaseClass::GetMetaClassStatic()); }\
\
private: \
\
static MetaClass *Class ## StaticMetaClass;\

class Component
{
	DECLARE_CLASS(Component)

	public:

		MetaClass* GetMetaClass() { return metaClass; }
		void SetMetaClass(MetaClass* NewMetaClass) { metaClass = NewMetaClass; }

		virtual void InitComponentDefaultProperties() {}

		virtual void RegisterComponent() {}
		virtual void UnRegisterComponent() {}

		Entity* GetOwner() { return Owner; }
		void SetOwner(Entity* NewOwner) { Owner = NewOwner; }

		template<typename T>
		static T* DynamicCast(Component* component)
		{
			if (component->GetMetaClass() == T::GetMetaClassStatic() || component->GetMetaClass()->IsBaseOf(T::GetMetaClassStatic()) || T::GetMetaClassStatic()->IsBaseOf(component->GetMetaClass())) return (T*)component;

			return nullptr;
		}

	protected:

		MetaClass *metaClass;

		Entity *Owner;

	private:

		
};