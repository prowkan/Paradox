#pragma once

#include "../../../Resource.h"

class MaterialResource : public Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) override;
		virtual void DestroyResource() override;

	private:
};