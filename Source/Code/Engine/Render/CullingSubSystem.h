#pragma once

class StaticMeshComponent;
class PointLightComponent;

class CullingSubSystem
{
	public:

		vector<StaticMeshComponent*> GetVisibleStaticMeshesInFrustum(const vector<StaticMeshComponent*>& InputStaticMeshes, const XMMATRIX& ViewProjMatrix);
		vector<PointLightComponent*> GetVisiblePointLightsInFrustum(const vector<PointLightComponent*>& InputPointLights, const XMMATRIX& ViewProjMatrix);

	private:

		void ExtractFrustumPlanesFromViewProjMatrix(const XMMATRIX& ViewProjMatrix, XMVECTOR* FrustumPlanes);

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);
		bool CullSphereVsFrustum(const XMVECTOR& SphereCenter, const float SphereRadius, const XMVECTOR* FrustumPlanes);
};