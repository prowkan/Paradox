#pragma once

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
	vector<Texture2DResource*> Textures;
};

class MaterialResource : public Resource
{
	public:

		virtual void CreateResource(const string& ResourceName, const void* ResourceData) override;
		virtual void DestroyResource() override;

		Texture2DResource* GetTexture(UINT Index) { return Textures[Index]; }

		RenderMaterial* GetRenderMaterial() { return renderMaterial; }

	private:

		RenderMaterial *renderMaterial;

		vector<Texture2DResource*> Textures;
};