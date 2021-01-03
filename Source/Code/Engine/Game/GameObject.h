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

	protected:

		MetaClass *metaClass;

		vector<Component*> Components;

		World *OwningWorld;

	private:
};