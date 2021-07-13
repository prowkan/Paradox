#pragma once

#include "../../../Resource.h"

struct RenderMesh;

struct StaticMeshResourceCreateInfo
{
	void *MeshData;
	UINT VertexCount;
	UINT IndexCount;
};

struct SubMeshData
{
	UINT IndexCount;
	UINT IndexOffset;
};

struct LODData
{
	UINT VertexCount;
	UINT IndexCount;
	UINT VertexOffset;
	UINT IndexOffset;
	UINT SubMeshesCount;
	UINT ElementOffset;
	SubMeshData SubMeshDataArray[3];
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
		UINT GetSubMeshCount(UINT LODLevel) { return LODDataArray[LODLevel].SubMeshesCount; }
		UINT GetIndexCount(UINT LODLevel, UINT SubMeshIndex) { return LODDataArray[LODLevel].SubMeshDataArray[SubMeshIndex].IndexCount; }
		UINT GetIndexOffset(UINT LODLevel, UINT SubMeshIndex) { return LODDataArray[LODLevel].SubMeshDataArray[SubMeshIndex].IndexOffset; }
		UINT GetElementOffset(UINT LODLevel) { return LODDataArray[LODLevel].ElementOffset; }

		UINT GetTotalElementsCount()
		{
			UINT ElementsCount = 0;

			for (UINT i = 0; i < LODCount; i++)
			{
				ElementsCount += LODDataArray[i].SubMeshesCount;
			}

			return ElementsCount;
		}

	private:

		RenderMesh *renderMesh;

		UINT LODCount;

		UINT VertexCount, IndexCount;

		static const UINT MAX_LODS_IN_STATIC_MESH = 5;

		LODData LODDataArray[MAX_LODS_IN_STATIC_MESH];
};