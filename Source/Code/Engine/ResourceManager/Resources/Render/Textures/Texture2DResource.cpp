#include "Texture2DResource.h"

#include <Engine/Engine.h>

void Texture2DResource::CreateResource(const void* ResourceData)
{
	Texture2DResourceCreateInfo& texture2DResourceCreateInfo = *(Texture2DResourceCreateInfo*)ResourceData;

	RenderTextureCreateInfo renderTextureCreateInfo;
	renderTextureCreateInfo.Height = texture2DResourceCreateInfo.Height;
	renderTextureCreateInfo.MIPLevels = texture2DResourceCreateInfo.MIPLevels;
	renderTextureCreateInfo.SRGB = texture2DResourceCreateInfo.SRGB;
	renderTextureCreateInfo.TexelData = texture2DResourceCreateInfo.TexelData;
	renderTextureCreateInfo.Width = texture2DResourceCreateInfo.Width;

	renderTexture = Engine::GetEngine().GetRenderSystem().CreateRenderTexture(renderTextureCreateInfo);
}

void Texture2DResource::DestroyResource()
{
}