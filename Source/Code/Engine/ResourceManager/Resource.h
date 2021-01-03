#pragma once

class Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) {}
		virtual void DestroyResource() {}

	private:
};