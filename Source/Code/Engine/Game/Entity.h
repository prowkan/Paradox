#pragma once

#include "MetaClass.h"

class Component;
class World;

class Entity
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) Entity(); }

		static MetaClass* GetMetaClassStatic() { return EntityMetaClass; }

		MetaClass* GetMetaClass() { return metaClass; }
		void SetMetaClass(MetaClass* NewMetaClass) { metaClass = NewMetaClass; }

		Component* CreateDefaultComponent(MetaClass* metaClass);

		virtual void InitDefaultProperties() {}

		World *GetWorld() { return OwningWorld; }
		void SetWorld(World* NewWorld) { OwningWorld = NewWorld; }

		template<typename T>
		T* GetComponent()
		{
			for (Component* component : Components)
			{
				T* ConcreteComponent = Component::DynamicCast<T>(component);

				if (ConcreteComponent != nullptr) return ConcreteComponent;
			}

			return nullptr;
		}

		template<typename T>
		static T* DynamicCast(Entity* gameObject)
		{
			if (gameObject->GetMetaClass() == T::GetMetaClassStatic() || gameObject->GetMetaClass()->IsBaseOf(T::GetMetaClassStatic()) || T::GetMetaClassStatic()->IsBaseOf(gameObject->GetMetaClass())) return (T*)gameObject;

			return nullptr;
		}

		static MetaClass *EntityMetaClass;

	protected:

		MetaClass *metaClass;

		vector<Component*> Components;

		World *OwningWorld;

	private:
};