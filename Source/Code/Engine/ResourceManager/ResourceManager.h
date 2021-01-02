#pragma once

class Resource;

class ResourceManager
{
	public:

		template<typename T>
		void AddResource(const string& ResourceName, const void* ResourceData)
		{
			T* resource = new T();
			resource->CreateResource(ResourceData);
			ResourceTable.insert(pair<string, Resource*>(ResourceName, resource));
		}

		template<typename T>
		T* GetResource(const string& ResourceName) { return (T*)ResourceTable[ResourceName]; }

	private:

		map<string, Resource*> ResourceTable;
};