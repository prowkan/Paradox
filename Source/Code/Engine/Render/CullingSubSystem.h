#pragma once

#include <Containers/DynamicArray.h>

class StaticMeshComponent;

class CullingSubSystem
{
	public:

		DynamicArray<StaticMeshComponent*> GetVisibleStaticMeshesInFrustum(const DynamicArray<StaticMeshComponent*>& InputStaticMeshes, const XMMATRIX& ViewProjMatrix);

	private:

		void ExtractFrustumPlanesFromViewProjMatrix(const XMMATRIX& ViewProjMatrix, XMVECTOR* FrustumPlanes);

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);
};