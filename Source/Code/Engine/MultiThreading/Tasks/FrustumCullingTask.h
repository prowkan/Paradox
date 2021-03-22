#pragma once

#include "../Task.h"

class StaticMeshComponent;

class FrustumCullingTask : public Task
{
	public:

		FrustumCullingTask(const vector<StaticMeshComponent*>& InputStaticMeshesArray, const XMVECTOR* FrustumPlanes, const size_t Begin, const size_t End, const bool DoOcclusionTest) : InputStaticMeshesArray(InputStaticMeshesArray)
		{
			this->FrustumPlanes = FrustumPlanes;
			this->Begin = Begin;
			this->End = End;
			this->DoOcclusionTest = DoOcclusionTest;
		}

		virtual void Execute(const UINT ThreadID) override;

		vector<StaticMeshComponent*>& GetOutputData() { return OutputStaticMeshesArray; }

	private:

		const vector<StaticMeshComponent*>& InputStaticMeshesArray;
		vector<StaticMeshComponent*> OutputStaticMeshesArray;

		const XMVECTOR *FrustumPlanes;
		size_t Begin, End;
		bool DoOcclusionTest;

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);
};