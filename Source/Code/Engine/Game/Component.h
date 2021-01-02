#pragma once

class MetaClass;
class Component;

extern MetaClass *ComponentMetaClass;

class Component
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) Component(); }

		static MetaClass* GetMetaClassStatic() { return ComponentMetaClass; }

		MetaClass* GetMetaClass() { return metaClass; }

		virtual void InitComponentDefaultProperties() {}

	protected:

		MetaClass *metaClass;

	private:
};