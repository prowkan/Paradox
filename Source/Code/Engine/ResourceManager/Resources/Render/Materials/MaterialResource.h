#pragma once

#include <Containers/DynamicArray.h>

#include "../../../Resource.h"

struct RenderMaterial;

class Texture2DResource;

struct MaterialResourceCreateInfo
{
	void* GBufferOpaquePassVertexShaderByteCodeData;
	void* GBufferOpaquePassPixelShaderByteCodeData;
	size_t GBufferOpaquePassVertexShaderByteCodeLength;
	size_t GBufferOpaquePassPixelShaderByteCodeLength;
	void* ShadowMapPassVertexShaderByteCodeData;
	void* ShadowMapPassPixelShaderByteCodeData;
	size_t ShadowMapPassVertexShaderByteCodeLength;
	size_t ShadowMapPassPixelShaderByteCodeLength;
	void* TransparentPassVertexShaderByteCodeData;
	void* TransparentPassPixelShaderByteCodeData;
	size_t TransparentPassVertexShaderByteCodeLength;
	size_t TransparentPassPixelShaderByteCodeLength;
	DynamicArray<Texture2DResource*> Textures;
	BYTE BlendMode;
};

class MaterialResource : public Resource
{
	public:

		virtual void CreateResource(const String& ResourceName, const void* ResourceData) override;
		virtual void DestroyResource() override;

		Texture2DResource* GetTexture(UINT Index) { return Textures[Index]; }

		RenderMaterial* GetRenderMaterial() { return renderMaterial; }

		BYTE GetBlendMode() { return BlendMode; }

		UINT GetTexturesCount() { return (UINT)Textures.GetLength(); }

	private:

		RenderMaterial *renderMaterial;

		DynamicArray<Texture2DResource*> Textures;

		BYTE BlendMode;
};