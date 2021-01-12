#pragma once

class MetaClass;
class Component;
class Entity;

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

		Entity* GetOwner() { return Owner; }
		void SetOwner(Entity* NewOwner) { Owner = NewOwner; }

		template<typename T>
		static T* DynamicCast(Component* component)
		{
			if (component->GetMetaClass() == T::GetMetaClassStatic() || component->GetMetaClass()->IsBaseOf(T::GetMetaClassStatic()) || T::GetMetaClassStatic()->IsBaseOf(component->GetMetaClass())) return (T*)component;

			return nullptr;
		}

		static MetaClass *ComponentMetaClass;

	protected:

		MetaClass *metaClass;

		Entity *Owner;

	private:
};