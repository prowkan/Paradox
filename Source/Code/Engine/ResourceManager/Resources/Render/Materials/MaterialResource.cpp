// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MaterialResource.h"

#include <Engine/Engine.h>

void MaterialResource::CreateResource(const void* ResourceData)
{
	MaterialResourceCreateInfo& materialResourceCreateInfo = *(MaterialResourceCreateInfo*)ResourceData;

	Textures = materialResourceCreateInfo.Textures;

	RenderMaterialCreateInfo renderMaterialCreateInfo;
	renderMaterialCreateInfo.PixelShaderByteCodeData = materialResourceCreateInfo.PixelShaderByteCodeData;
	renderMaterialCreateInfo.PixelShaderByteCodeLength = materialResourceCreateInfo.PixelShaderByteCodeLength;
	renderMaterialCreateInfo.VertexShaderByteCodeData = materialResourceCreateInfo.VertexShaderByteCodeData;
	renderMaterialCreateInfo.VertexShaderByteCodeLength = materialResourceCreateInfo.VertexShaderByteCodeLength;

	renderMaterial = Engine::GetEngine().GetRenderSystem().CreateRenderMaterial(renderMaterialCreateInfo);
}

void MaterialResource::DestroyResource()
{
	Engine::GetEngine().GetRenderSystem().DestroyRenderMaterial(renderMaterial);
}