#pragma once

#include <Containers/DynamicArray.h>

#include "../Task.h"

class StaticMeshComponent;

class FrustumCullingTask : public Task
{
	public:

		struct CullingResultItem
		{
			StaticMeshComponent *staticMeshComponent;
			UINT Index;
		};

		FrustumCullingTask(const DynamicArray<StaticMeshComponent*>& InputStaticMeshesArray, const XMVECTOR* FrustumPlanes, const size_t Begin, const size_t End, const bool DoOcclusionTest) : InputStaticMeshesArray(InputStaticMeshesArray)
		{
			this->FrustumPlanes = FrustumPlanes;
			this->Begin = Begin;
			this->End = End;
			this->DoOcclusionTest = DoOcclusionTest;
		}

		virtual void Execute(const UINT ThreadID) override;

		DynamicArray<CullingResultItem>& GetOutputData() { return OutputStaticMeshesArray; }

	private:

		const DynamicArray<StaticMeshComponent*>& InputStaticMeshesArray;
		DynamicArray<CullingResultItem> OutputStaticMeshesArray;

		const XMVECTOR *FrustumPlanes;
		size_t Begin, End;
		bool DoOcclusionTest;

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);
};