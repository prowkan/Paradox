#pragma once

#include <Containers/DynamicArray.h>

#include "MetaClass.h"

class Component;
class World;
class Resource;

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
			for (Component* component : Components)
			{
				T* ConcreteComponent = Component::DynamicCast<T>(component);

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

		virtual void LoadFromFile(HANDLE File) {}

		const char* GetEntityName() { return EntityName; }
		const char *EntityName;

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

		DynamicArray<Component*> Components;

		World *OwningWorld;

		//const char *EntityName;

	private:
};