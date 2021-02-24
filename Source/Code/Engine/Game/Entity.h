#pragma once

#include <Containers/DynamicArray.h>

#include "MetaClass.h"

class Component;
class World;

class Entity
{
	DECLARE_CLASS(Entity)

	public:

		MetaClass* GetMetaClass() { return metaClass; }
		void SetMetaClass(MetaClass* NewMetaClass) { metaClass = NewMetaClass; }

		Component* CreateDefaultComponent(MetaClass* metaClass);

		virtual void InitDefaultProperties() {}

		World *GetWorld() { return OwningWorld; }
		void SetWorld(World* NewWorld) { OwningWorld = NewWorld; }

		template<typename T>
		T* GetComponent()
		{
			/*for (Component* component : Components)
			{
				T* ConcreteComponent = Component::DynamicCast<T>(component);

				if (ConcreteComponent != nullptr) return ConcreteComponent;
			}*/

			for (int i = 0; i < Components.GetLength(); i++)
			{
				T* ConcreteComponent = Component::DynamicCast<T>(Components[i]);

				if (ConcreteComponent != nullptr) return ConcreteComponent;
			}

			return nullptr;
		}

		template<typename T>
		static T* DynamicCast(Entity* entity)
		{
			if (entity->GetMetaClass() == T::GetMetaClassStatic() || entity->GetMetaClass()->IsBaseOf(T::GetMetaClassStatic()) || T::GetMetaClassStatic()->IsBaseOf(entity->GetMetaClass())) return (T*)entity;

			return nullptr;
		}

	protected:

		MetaClass *metaClass;

		DynamicArray<Component*> Components;

		World *OwningWorld;

	private:
};