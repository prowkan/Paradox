// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "StaticMeshResource.h"

#include <Engine/Engine.h>

void StaticMeshResource::CreateResource(const String& ResourceName, const void* ResourceData)
{
	Resource::CreateResource(ResourceName, ResourceData);

	StaticMeshResourceCreateInfo& staticMeshResourceCreateInfo = *(StaticMeshResourceCreateInfo*)ResourceData;

	VertexCount = staticMeshResourceCreateInfo.VertexCount;
	IndexCount = staticMeshResourceCreateInfo.IndexCount;

	RenderMeshCreateInfo renderMeshCreateInfo;
	renderMeshCreateInfo.IndexCount = staticMeshResourceCreateInfo.IndexCount;
	renderMeshCreateInfo.VertexCount = staticMeshResourceCreateInfo.VertexCount;
	renderMeshCreateInfo.MeshData = staticMeshResourceCreateInfo.MeshData;

	renderMesh = Engine::GetEngine().GetRenderSystem().CreateRenderMesh(renderMeshCreateInfo);

	LODDataArray[0].VertexCount = 33 * 33 * 6;
	LODDataArray[0].IndexCount = 32 * 32 * 6 * 6;
	LODDataArray[0].VertexOffset = 0;
	LODDataArray[0].IndexOffset = 0;

	LODDataArray[1].VertexCount = 17 * 17 * 6;
	LODDataArray[1].IndexCount = 16 * 16 * 6 * 6;
	LODDataArray[1].VertexOffset = LODDataArray[0].VertexCount;
	LODDataArray[1].IndexOffset = LODDataArray[0].IndexCount;

	LODDataArray[2].VertexCount = 9 * 9 * 6;
	LODDataArray[2].IndexCount = 8 * 8 * 6 * 6;
	LODDataArray[2].VertexOffset = LODDataArray[0].VertexCount + LODDataArray[1].VertexCount;
	LODDataArray[2].IndexOffset = LODDataArray[0].IndexCount + LODDataArray[1].IndexCount;

	LODDataArray[3].VertexCount = 5 * 5 * 6;
	LODDataArray[3].IndexCount = 4 * 4 * 6 * 6;
	LODDataArray[3].VertexOffset = LODDataArray[0].VertexCount + LODDataArray[1].VertexCount + LODDataArray[2].VertexCount;
	LODDataArray[3].IndexOffset = LODDataArray[0].IndexCount + LODDataArray[1].IndexCount + LODDataArray[2].IndexCount;

	LODDataArray[4].VertexCount = 3 * 3 * 6;
	LODDataArray[4].IndexCount = 2 * 2 * 6 * 6;
	LODDataArray[4].VertexOffset = LODDataArray[0].VertexCount + LODDataArray[1].VertexCount + LODDataArray[2].VertexCount + LODDataArray[3].VertexCount;
	LODDataArray[4].IndexOffset = LODDataArray[0].IndexCount + LODDataArray[1].IndexCount + LODDataArray[2].IndexCount + LODDataArray[3].IndexCount;
}

void StaticMeshResource::DestroyResource()
{
	Engine::GetEngine().GetRenderSystem().DestroyRenderMesh(renderMesh);
}