#pragma once

#include "../../../Resource.h"

struct RenderMaterial;

class Texture2DResource;

struct MaterialResourceCreateInfo
{
	void *VertexShaderByteCodeData;
	void *PixelShaderByteCodeData;
	size_t VertexShaderByteCodeLength;
	size_t PixelShaderByteCodeLength;
	vector<Texture2DResource*> Textures;
};

class MaterialResource : public Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) override;
		virtual void DestroyResource() override;

		Texture2DResource* GetTexture(UINT Index) { return Textures[0]; }

		RenderMaterial* GetRenderMaterial() { return renderMaterial; }

	private:

		RenderMaterial *renderMaterial;

		vector<Texture2DResource*> Textures;
};