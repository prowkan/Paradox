#pragma once

#include "../../../Resource.h"

struct RenderMaterial;

class Texture2DResource;

struct MaterialResourceCreateInfo
{
	UINT VertexShaderByteCodeLength;
	void *VertexShaderByteCodeData;
	UINT PixelShaderByteCodeLength;
	void *PixelShaderByteCodeData;
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