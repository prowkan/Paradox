#pragma once

#include "../../../Resource.h"

class Texture2DResource : public Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) override;
		virtual void DestroyResource() override;

	private:
};