#pragma once

class MetaClass;
class Component;
class GameObject;

extern MetaClass *ComponentMetaClass;

class Component
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) Component(); }

		static MetaClass* GetMetaClassStatic() { return ComponentMetaClass; }

		MetaClass* GetMetaClass() { return metaClass; }
		void SetMetaClass(MetaClass* NewMetaClass) { metaClass = NewMetaClass; }

		virtual void InitComponentDefaultProperties() {}

		virtual void RegisterComponent() {}
		virtual void UnRegisterComponent() {}

		GameObject* GetOwner() { return Owner; }
		void SetOwner(GameObject* NewOwner) { Owner = NewOwner; }

	protected:

		MetaClass *metaClass;

		GameObject *Owner;

	private:
};