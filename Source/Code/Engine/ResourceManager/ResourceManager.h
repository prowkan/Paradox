#pragma once

#include <Containers/String.h>
#include <Containers/HashTable.h>

#include "Resource.h"

class ResourceManager
{
	public:

		template<typename T>
		void AddResource(const String& ResourceName, const void* ResourceData)
		{
			T* resource = new T();
			resource->CreateResource(ResourceName, ResourceData);
			ResourceTable.Insert(ResourceName, resource);
		}

		template<typename T>
		T* GetResource(const String& ResourceName) { return (T*)ResourceTable[ResourceName]; }

		void DestroyAllResources()
		{
			int i = 0;

			for (auto& It : ResourceTable)
			{
				It->Value->DestroyResource();
				i++;
			}

			int a = i;

			//ResourceTable.clear();
		}

	private:

		HashTable<String, Resource*> ResourceTable;
};