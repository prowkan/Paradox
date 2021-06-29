#pragma once

#include <Containers/DynamicArray.h>

#include "../../../Resource.h"

struct RenderMaterial;

class Texture2DResource;

struct MaterialResourceCreateInfo
{
	void *GBufferOpaquePassVertexShaderByteCodeData;
	void *GBufferOpaquePassPixelShaderByteCodeData;
	size_t GBufferOpaquePassVertexShaderByteCodeLength;
	size_t GBufferOpaquePassPixelShaderByteCodeLength;
	void *ShadowMapPassVertexShaderByteCodeData;
	void *ShadowMapPassPixelShaderByteCodeData;
	size_t ShadowMapPassVertexShaderByteCodeLength;
	size_t ShadowMapPassPixelShaderByteCodeLength;
	DynamicArray<Texture2DResource*> Textures;
};

class MaterialResource : public Resource
{
	public:

		virtual void CreateResource(const String& ResourceName, const void* ResourceData) override;
		virtual void DestroyResource() override;

		Texture2DResource* GetTexture(UINT Index) { return Textures[Index]; }

		RenderMaterial* GetRenderMaterial() { return renderMaterial; }

	private:

		RenderMaterial *renderMaterial;

		DynamicArray<Texture2DResource*> Textures;
};