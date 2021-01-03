#pragma once

#include "../../../Resource.h"

struct RenderTexture;

struct Texture2DResourceCreateInfo
{
	UINT Width, Height;
	UINT MIPLevels;
	BOOL SRGB;
	BYTE *TexelData;
};

class Texture2DResource : public Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) override;
		virtual void DestroyResource() override;

		RenderTexture* GetRenderTexture() { return renderTexture; }

	private:

		RenderTexture *renderTexture;
};