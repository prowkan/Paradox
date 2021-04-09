#pragma once

class Resource
{
	public:

		virtual void CreateResource(const string& ResourceName, const void* ResourceData) { this->ResourceName = ResourceName; }
		virtual void DestroyResource() {}

		const string& GetResourceName() { return ResourceName; }

	private:

		string ResourceName;
};