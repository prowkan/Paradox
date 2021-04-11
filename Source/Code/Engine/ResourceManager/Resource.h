#pragma once

#include <Containers/String.h>

class Resource
{
	public:

		virtual void CreateResource(const String& ResourceName, const void* ResourceData) { this->ResourceName = ResourceName; }
		virtual void DestroyResource() {}

		const String& GetResourceName() { return ResourceName; }

	private:

		String ResourceName;
};