#pragma once

#include "Resource.h"

class ResourceManager
{
	public:

		template<typename T>
		void AddResource(const string& ResourceName, const void* ResourceData)
		{
			T* resource = new T();
			resource->CreateResource(ResourceData);
			ResourceTable.emplace(ResourceName, resource);
		}

		template<typename T>
		T* GetResource(const string& ResourceName) { return (T*)ResourceTable[ResourceName]; }

		void DestroyAllResources()
		{
			for (auto& It : ResourceTable)
			{
				It.second->DestroyResource();
			}

			ResourceTable.clear();
		}

	private:

		map<string, Resource*> ResourceTable;
};