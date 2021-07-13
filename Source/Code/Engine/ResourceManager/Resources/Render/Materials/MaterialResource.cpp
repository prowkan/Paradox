// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MaterialResource.h"

#include <Engine/Engine.h>

void MaterialResource::CreateResource(const String& ResourceName, const void* ResourceData)
{
	Resource::CreateResource(ResourceName, ResourceData);

	MaterialResourceCreateInfo& materialResourceCreateInfo = *(MaterialResourceCreateInfo*)ResourceData;

	Textures = materialResourceCreateInfo.Textures;

	RenderMaterialCreateInfo renderMaterialCreateInfo;
	if (materialResourceCreateInfo.BlendMode == 0)
	{
		renderMaterialCreateInfo.Opaque.GBufferOpaquePassPixelShaderByteCodeData = materialResourceCreateInfo.Opaque.GBufferOpaquePassPixelShaderByteCodeData;
		renderMaterialCreateInfo.Opaque.GBufferOpaquePassPixelShaderByteCodeLength = materialResourceCreateInfo.Opaque.GBufferOpaquePassPixelShaderByteCodeLength;
		renderMaterialCreateInfo.Opaque.GBufferOpaquePassVertexShaderByteCodeData = materialResourceCreateInfo.Opaque.GBufferOpaquePassVertexShaderByteCodeData;
		renderMaterialCreateInfo.Opaque.GBufferOpaquePassVertexShaderByteCodeLength = materialResourceCreateInfo.Opaque.GBufferOpaquePassVertexShaderByteCodeLength;
		renderMaterialCreateInfo.Opaque.ShadowMapPassPixelShaderByteCodeData = materialResourceCreateInfo.Opaque.ShadowMapPassPixelShaderByteCodeData;
		renderMaterialCreateInfo.Opaque.ShadowMapPassPixelShaderByteCodeLength = materialResourceCreateInfo.Opaque.ShadowMapPassPixelShaderByteCodeLength;
		renderMaterialCreateInfo.Opaque.ShadowMapPassVertexShaderByteCodeData = materialResourceCreateInfo.Opaque.ShadowMapPassVertexShaderByteCodeData;
		renderMaterialCreateInfo.Opaque.ShadowMapPassVertexShaderByteCodeLength = materialResourceCreateInfo.Opaque.ShadowMapPassVertexShaderByteCodeLength;
	}
	else if (materialResourceCreateInfo.BlendMode == 1)
	{
		renderMaterialCreateInfo.Transparent.TransparentPassPixelShaderByteCodeData = materialResourceCreateInfo.Transparent.TransparentPassPixelShaderByteCodeData;
		renderMaterialCreateInfo.Transparent.TransparentPassPixelShaderByteCodeLength = materialResourceCreateInfo.Transparent.TransparentPassPixelShaderByteCodeLength;
		renderMaterialCreateInfo.Transparent.TransparentPassVertexShaderByteCodeData = materialResourceCreateInfo.Transparent.TransparentPassVertexShaderByteCodeData;
		renderMaterialCreateInfo.Transparent.TransparentPassVertexShaderByteCodeLength = materialResourceCreateInfo.Transparent.TransparentPassVertexShaderByteCodeLength;
	}
	renderMaterialCreateInfo.BlendMode = materialResourceCreateInfo.BlendMode;

	BlendMode = materialResourceCreateInfo.BlendMode;

	renderMaterial = Engine::GetEngine().GetRenderSystem().CreateRenderMaterial(renderMaterialCreateInfo);
}

void MaterialResource::DestroyResource()
{
	Engine::GetEngine().GetRenderSystem().DestroyRenderMaterial(renderMaterial);
}