#pragma once

class StaticMeshComponent;

class CullingSubSystem
{
	public:

		vector<StaticMeshComponent*> GetVisibleStaticMeshesInFrustum(const vector<StaticMeshComponent*>& InputStaticMeshes, const XMMATRIX& ViewProjMatrix);

	private:

		void ExtractFrustumPlanesFromViewProjMatrix(const XMMATRIX& ViewProjMatrix, XMVECTOR* FrustumPlanes);

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);
};