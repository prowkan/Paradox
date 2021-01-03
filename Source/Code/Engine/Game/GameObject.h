#pragma once

class MetaClass;
class Component;
class World;

extern MetaClass *GameObjectMetaClass;

class GameObject
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) GameObject(); }

		static MetaClass* GetMetaClassStatic() { return GameObjectMetaClass; }

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
				T* ConcreteComponent = dynamic_cast<T*>(component);

				if (ConcreteComponent != nullptr) return ConcreteComponent;
			}

			return nullptr;
		}

	protected:

		MetaClass *metaClass;

		vector<Component*> Components;

		World *OwningWorld;

	private:
};