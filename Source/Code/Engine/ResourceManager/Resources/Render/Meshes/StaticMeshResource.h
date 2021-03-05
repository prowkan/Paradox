#pragma once

#include "../../../Resource.h"

struct RenderMesh;

struct StaticMeshResourceCreateInfo
{
	void *VertexData;
	void *IndexData;
	UINT VertexCount;
	UINT IndexCount;
};

class StaticMeshResource : public Resource
{
	public:

		virtual void CreateResource(const void* ResourceData) override;
		virtual void DestroyResource() override;

		RenderMesh* GetRenderMesh() { return renderMesh; }

	private:

		RenderMesh *renderMesh;
};