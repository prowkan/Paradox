// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MaterialResource.h"

#include <Engine/Engine.h>

void MaterialResource::CreateResource(const void* ResourceData)
{
	MaterialResourceCreateInfo& materialResourceCreateInfo = *(MaterialResourceCreateInfo*)ResourceData;

	Textures = materialResourceCreateInfo.Textures;

	RenderMaterialCreateInfo renderMaterialCreateInfo;
	renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData;
	renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength;
	renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData;
	renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength;
	renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeData = materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData;
	renderMaterialCreateInfo.ShadowMapPassPixelShaderByteCodeLength = materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength;
	renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeData = materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData;
	renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeLength = materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength;

	renderMaterial = Engine::GetEngine().GetRenderSystem().CreateRenderMaterial(renderMaterialCreateInfo);
}

void MaterialResource::DestroyResource()
{
	Engine::GetEngine().GetRenderSystem().DestroyRenderMaterial(renderMaterial);
}