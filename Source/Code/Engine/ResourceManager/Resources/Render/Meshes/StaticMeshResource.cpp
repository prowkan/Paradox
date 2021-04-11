// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "StaticMeshResource.h"

#include <Engine/Engine.h>

void StaticMeshResource::CreateResource(const String& ResourceName, const void* ResourceData)
{
	Resource::CreateResource(ResourceName, ResourceData);

	StaticMeshResourceCreateInfo& staticMeshResourceCreateInfo = *(StaticMeshResourceCreateInfo*)ResourceData;

	RenderMeshCreateInfo renderMeshCreateInfo;
	renderMeshCreateInfo.IndexCount = staticMeshResourceCreateInfo.IndexCount;
	renderMeshCreateInfo.VertexCount = staticMeshResourceCreateInfo.VertexCount;
	renderMeshCreateInfo.MeshData = staticMeshResourceCreateInfo.MeshData;

	renderMesh = Engine::GetEngine().GetRenderSystem().CreateRenderMesh(renderMeshCreateInfo);
}

void StaticMeshResource::DestroyResource()
{
	Engine::GetEngine().GetRenderSystem().DestroyRenderMesh(renderMesh);
}