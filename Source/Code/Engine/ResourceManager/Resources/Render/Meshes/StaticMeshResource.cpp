#include "StaticMeshResource.h"

#include <Engine/Engine.h>

void StaticMeshResource::CreateResource(const void* ResourceData)
{
	StaticMeshResourceCreateInfo& staticMeshResourceCreateInfo = *(StaticMeshResourceCreateInfo*)ResourceData;

	RenderMeshCreateInfo renderMeshCreateInfo;
	renderMeshCreateInfo.IndexCount = staticMeshResourceCreateInfo.IndexCount;
	renderMeshCreateInfo.IndexData = staticMeshResourceCreateInfo.IndexData;
	renderMeshCreateInfo.VertexCount = staticMeshResourceCreateInfo.VertexCount;
	renderMeshCreateInfo.VertexData = staticMeshResourceCreateInfo.VertexData;

	renderMesh = Engine::GetEngine().GetRenderSystem().CreateRenderMesh(renderMeshCreateInfo);
}

void StaticMeshResource::DestroyResource()
{
}