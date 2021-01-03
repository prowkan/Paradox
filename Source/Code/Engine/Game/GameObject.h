#pragma once

class MetaClass;
class Component;

extern MetaClass *GameObjectMetaClass;

class GameObject
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) GameObject(); }

		static MetaClass* GetMetaClassStatic() { return GameObjectMetaClass; }

		MetaClass* GetMetaClass() { return metaClass; }

		Component* CreateDefaultComponent(MetaClass* metaClass);

		virtual void InitDefaultProperties() {}

	protected:

		MetaClass *metaClass;

		vector<Component*> Components;

	private:
};