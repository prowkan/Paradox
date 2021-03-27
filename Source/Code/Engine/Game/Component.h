#pragma once

#include "MetaClass.h"

class Entity;

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

		virtual void LoadFromFile(HANDLE File) {}

	protected:

		MetaClass *metaClass;

		Entity *Owner;

	private:

		
};