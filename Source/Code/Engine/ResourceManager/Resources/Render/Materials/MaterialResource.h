#pragma once

#include <Containers/DynamicArray.h>

#include "../../../Resource.h"

struct RenderMaterial;

class Texture2DResource;

struct MaterialResourceCreateInfo
{
	void *VertexShaderByteCodeData;
	void *PixelShaderByteCodeData;
	size_t VertexShaderByteCodeLength;
	size_t PixelShaderByteCodeLength;
	DynamicArray<Texture2DResource*> Textures;
};

class MaterialResource : public Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) override;
		virtual void DestroyResource() override;

		Texture2DResource* GetTexture(UINT Index) { return Textures[Index]; }

		RenderMaterial* GetRenderMaterial() { return renderMaterial; }

	private:

		RenderMaterial *renderMaterial;

		DynamicArray<Texture2DResource*> Textures;
};