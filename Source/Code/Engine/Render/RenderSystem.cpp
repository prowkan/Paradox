// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include "DX11/RenderDeviceDX11.h"
#include "DX12/RenderDeviceDX12.h"

void RenderSystem::InitSystem()
{
	int CommandLineArgsCount;
	LPWSTR CommandLine = GetCommandLineW();
	LPWSTR *CommandLineArgs = CommandLineToArgvW(CommandLine, &CommandLineArgsCount);

	bool UseDirectX12 = false;

	for (int i = 0; i < CommandLineArgsCount; i++)
	{
		if (wcscmp(CommandLineArgs[i], (const wchar_t*)u"-DX12") == 0)
		{
			UseDirectX12 = true;
			break;
		}
	}

	if (UseDirectX12)
	{
		renderDevice = new RenderDeviceDX12();
	}
	else
	{
		renderDevice = new RenderDeviceDX11();
	}

	renderDevice->InitDevice();
}

void RenderSystem::ShutdownSystem()
{
	renderDevice->ShutdownDevice();
	
	if (renderDevice->GetDirectXVersion() == DirectXVersion::DirectX11)
		delete (RenderDeviceDX11*)renderDevice;
	else
		delete (RenderDeviceDX12*)renderDevice;
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