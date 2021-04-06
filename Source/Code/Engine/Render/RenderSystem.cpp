// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include "RenderDevice.h"

#include "D3D12/RenderDeviceD3D12.h"
#include "Vulkan/RenderDeviceVulkan.h"

#include <Engine/Engine.h>

void RenderSystem::InitSystem()
{
	clusterizationSubSystem.PreComputeClustersPlanes();

	string GraphicsAPI = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueString("System", "GraphicsAPI");

	if (GraphicsAPI == "D3D12")
		renderDevice = new RenderDeviceD3D12();
	else if (GraphicsAPI == "Vulkan")
		renderDevice = new RenderDeviceVulkan();
	else
		renderDevice = nullptr;

	renderDevice->InitDevice();
}

void RenderSystem::ShutdownSystem()
{
	renderDevice->ShutdownDevice();

	string GraphicsAPI = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueString("System", "GraphicsAPI");
		
	if (GraphicsAPI == "D3D12")
		delete (RenderDeviceD3D12*)renderDevice;
	else if (GraphicsAPI == "Vulkan")
		delete (RenderDeviceVulkan*)renderDevice;
}

void RenderSystem::TickSystem(float DeltaTime)
{
	renderDevice->TickDevice(DeltaTime);	
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