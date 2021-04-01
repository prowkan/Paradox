#pragma once

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

		uint32_t GetPropertiesCount() { return (uint32_t)metaClass->ClassProperties.size(); }

		const char* GetPropertyName(uint32_t PropertyIndex)
		{
			auto PropertyIterator = metaClass->ClassProperties.begin();

			while (PropertyIndex > 0)
			{
				++PropertyIterator;
				--PropertyIndex;
			}

			return (*PropertyIterator).first.c_str();
		}

		ClassPropertyType GetPropertyType(uint32_t PropertyIndex)
		{
			auto PropertyIterator = metaClass->ClassProperties.begin();

			while (PropertyIndex > 0)
			{
				++PropertyIterator;
				--PropertyIndex;
			}

			return (*PropertyIterator).second->PropertyType;
		}

		float GetFloatProperty(const string& PropertyName) { return *(float*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetFloatProperty(const string& PropertyName, const float Value) { *(float*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		XMFLOAT3 GetVectorProperty(const string& PropertyName) { return *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetVectorProperty(const string& PropertyName, const XMFLOAT3& Value) { *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		XMFLOAT3 GetRotatorProperty(const string& PropertyName) { return *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetRotatorProperty(const string& PropertyName, const XMFLOAT3& Value) { *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		XMFLOAT3 GetColorProperty(const string& PropertyName) { return *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetColorProperty(const string& PropertyName, const XMFLOAT3& Value) { *(XMFLOAT3*)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		Entity* GetEntityReferenceProperty(const string& PropertyName) { return *(Entity**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetEntityReferenceProperty(const string& PropertyName, Entity* Value) { *(Entity**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		Component* GetComponentReferenceProperty(const string& PropertyName) { return *(Component**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetComponentReferenceProperty(const string& PropertyName, Component* Value) { *(Component**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

		Resource* GetResourceReferenceProperty(const string& PropertyName) { return *(Resource**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset); }
		void SetResourceReferenceProperty(const string& PropertyName, Resource* Value) { *(Resource**)((BYTE*)this + metaClass->ClassProperties[PropertyName]->ValueOffset) = Value; }

	protected:

		MetaClass *metaClass;

		vector<Component*> Components;

		World *OwningWorld;

		//const char *EntityName;

	private:
};