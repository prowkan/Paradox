#pragma once

#include "../../../Resource.h"

struct RenderMesh;

struct StaticMeshResourceCreateInfo
{
	void *MeshData;
	UINT VertexCount;
	UINT IndexCount;
};

struct LODData
{
	UINT VertexCount;
	UINT IndexCount;
	UINT VertexOffset;
	UINT IndexOffset;
};

class StaticMeshResource : public Resource
{
	public:

		virtual void CreateResource(const String& ResourceName, const void* ResourceData) override;
		virtual void DestroyResource() override;

		RenderMesh* GetRenderMesh() { return renderMesh; }

		UINT GetVertexCount() { return VertexCount; }
		UINT GetIndexCount() { return IndexCount; }
		UINT GetVertexCount(UINT LODLevel) { return LODDataArray[LODLevel].VertexCount; }
		UINT GetIndexCount(UINT LODLevel) { return LODDataArray[LODLevel].IndexCount; }
		UINT GetVertexOffset(UINT LODLevel) { return LODDataArray[LODLevel].VertexOffset; }
		UINT GetIndexOffset(UINT LODLevel) { return LODDataArray[LODLevel].IndexOffset; }

	private:

		RenderMesh *renderMesh;

		UINT LODCount;

		UINT VertexCount, IndexCount;

		static const UINT MAX_LODS_IN_STATIC_MESH = 5;

		LODData LODDataArray[MAX_LODS_IN_STATIC_MESH];
};