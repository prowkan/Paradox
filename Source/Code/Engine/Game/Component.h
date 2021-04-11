#pragma once

#include "MetaClass.h"

class Entity;
class Resource;

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

		const char* GetComponentName() { return ComponentName; }
		const char *ComponentName;

		uint32_t GetPropertiesCount() {	return (uint32_t)metaClass->ClassProperties.GetSize(); }

		const char* GetPropertyName(uint32_t PropertyIndex)
		{
			HashTable<String, ClassProperty*>::Iterator PropertyIterator = metaClass->ClassProperties.Begin();

			while (PropertyIndex > 0)
			{
				++PropertyIterator;
				--PropertyIndex;
			}

			return PropertyIterator.GetTableNode()->Key.GetData();
		}

		ClassPropertyType GetPropertyType(uint32_t PropertyIndex)
		{
			HashTable<String, ClassProperty*>::Iterator PropertyIterator = metaClass->ClassProperties.Begin();

			while (PropertyIndex > 0)
			{
				++PropertyIterator;
				--PropertyIndex;
			}

			return PropertyIterator.GetTableNode()->Value->PropertyType;
		}

		float GetFloatProperty(const String& PropertyName) { return *(float*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetFloatProperty(const String& PropertyName, const float Value) { *(float*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		XMFLOAT3 GetVectorProperty(const String& PropertyName) { return *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetVectorProperty(const String& PropertyName, const XMFLOAT3& Value) { *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		XMFLOAT3 GetRotatorProperty(const String& PropertyName) { return *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetRotatorProperty(const String& PropertyName, const XMFLOAT3& Value) { *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		XMFLOAT3 GetColorProperty(const String& PropertyName) { return *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetColorProperty(const String& PropertyName, const XMFLOAT3& Value) { *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		Entity* GetEntityReferenceProperty(const String& PropertyName) { return *(Entity**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetEntityReferenceProperty(const String& PropertyName, Entity* Value) { *(Entity**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		Component* GetComponentReferenceProperty(const String& PropertyName) { return *(Component**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetComponentReferenceProperty(const String& PropertyName, Component* Value) { *(Component**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		Resource* GetResourceReferenceProperty(const String& PropertyName) { return *(Resource**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetResourceReferenceProperty(const String& PropertyName, Resource* Value) { *(Resource**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

	protected:

		MetaClass *metaClass;

		Entity *Owner;

	private:

		
};