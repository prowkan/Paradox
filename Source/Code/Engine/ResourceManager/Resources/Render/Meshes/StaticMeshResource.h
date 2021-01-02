#pragma once

#include "../../../Resource.h"

class StaticMeshResource : public Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) override;
		virtual void DestroyResource() override;

	private:
};