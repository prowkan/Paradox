#pragma once

class StaticMeshComponent;

class CullingSubSystem
{
	public:

		vector<StaticMeshComponent*> GetVisibleStaticMeshesInFrustum(const vector<StaticMeshComponent*>& InputStaticMeshes, const XMMATRIX& ViewProjMatrix);

		float* GetOcclusionBufferData() { return OcclusionBufferData; }

	private:

		void ExtractFrustumPlanesFromViewProjMatrix(const XMMATRIX& ViewProjMatrix, XMVECTOR* FrustumPlanes);

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);

		float OcclusionBufferData[256 * 144];
};