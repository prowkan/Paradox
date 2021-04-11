#pragma once

#include "../../../Resource.h"

struct RenderTexture;

enum class BlockCompression;

struct Texture2DResourceCreateInfo
{
	UINT Width, Height;
	UINT MIPLevels;
	BOOL SRGB;
	BOOL Compressed;
	BlockCompression CompressionType;
	BYTE *TexelData;
};

class Texture2DResource : public Resource
{
	public:

		virtual void CreateResource(const String& ResourceName, const void* ResourceData) override;
		virtual void DestroyResource() override;

		RenderTexture* GetRenderTexture() { return renderTexture; }

	private:

		RenderTexture *renderTexture;
};