// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include "DX11/RenderDeviceDX11.h"
#include "DX12/RenderDeviceDX12.h"

void RenderSystem::InitSystem()
{
	renderDevice = new RenderDeviceDX12();
	renderDevice->InitSystem();
}

void RenderSystem::ShutdownSystem()
{
	renderDevice->ShutdownSystem();
	delete renderDevice;
}

void RenderSystem::TickSystem(float DeltaTime)
{
	renderDevice->TickSystem(DeltaTime);
}

RenderMesh* RenderSystem::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	return renderDevice->CreateRenderMesh(renderMeshCreateInfo);
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	return renderDevice->CreateRenderTexture(renderTextureCreateInfo);
}

RenderMaterial* RenderSystem::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	return renderDevice->CreateRenderMaterial(renderMaterialCreateInfo);
}

void RenderSystem::DestroyRenderMesh(RenderMesh* renderMesh)
{
	renderDevice->DestroyRenderMesh(renderMesh);
}

void RenderSystem::DestroyRenderTexture(RenderTexture* renderTexture)
{
	renderDevice->DestroyRenderTexture(renderTexture);
}

void RenderSystem::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	renderDevice->DestroyRenderMaterial(renderMaterial);
}