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
		renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData;
		renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength;
		renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData;
		renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength;
		renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeData = materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData;
		renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeLength = materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength;
		renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeData = materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData;
		renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeLength = materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength;
	}
	else if (materialResourceCreateInfo.BlendMode == 1)
	{
		renderMaterialCreateInfo.TransparentPassPixelShaderByteCodeData = materialResourceCreateInfo.TransparentPassPixelShaderByteCodeData;
		renderMaterialCreateInfo.TransparentPassPixelShaderByteCodeLength = materialResourceCreateInfo.TransparentPassPixelShaderByteCodeLength;
		renderMaterialCreateInfo.TransparentPassVertexShaderByteCodeData = materialResourceCreateInfo.TransparentPassVertexShaderByteCodeData;
		renderMaterialCreateInfo.TransparentPassVertexShaderByteCodeLength = materialResourceCreateInfo.TransparentPassVertexShaderByteCodeLength;
	}
	renderMaterialCreateInfo.BlendMode = materialResourceCreateInfo.BlendMode;

	BlendMode = materialResourceCreateInfo.BlendMode;

	renderMaterial = Engine::GetEngine().GetRenderSystem().CreateRenderMaterial(renderMaterialCreateInfo);
}

void MaterialResource::DestroyResource()
{
	Engine::GetEngine().GetRenderSystem().DestroyRenderMaterial(renderMaterial);
}