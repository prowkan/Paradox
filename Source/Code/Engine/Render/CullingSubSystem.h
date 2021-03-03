#pragma once

class StaticMeshComponent;
class PointLightComponent;

class CullingSubSystem
{
	public:

		vector<StaticMeshComponent*> GetVisibleStaticMeshesInFrustum(const vector<StaticMeshComponent*>& InputStaticMeshes, const XMMATRIX& ViewProjMatrix, const bool DoOcclusionTest);
		vector<PointLightComponent*> GetVisiblePointLightsInFrustum(const vector<PointLightComponent*>& InputPointLights, const XMMATRIX& ViewProjMatrix);

		float* GetOcclusionBufferData() { return OcclusionBufferData; }

	private:

		void ExtractFrustumPlanesFromViewProjMatrix(const XMMATRIX& ViewProjMatrix, XMVECTOR* FrustumPlanes);

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);
		bool CullSphereVsFrustum(const XMVECTOR& SphereCenter, const float SphereRadius, const XMVECTOR* FrustumPlanes);

		float OcclusionBufferData[256 * 144];
};