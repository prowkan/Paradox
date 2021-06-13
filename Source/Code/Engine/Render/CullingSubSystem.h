#pragma once

#include <Containers/DynamicArray.h>

class StaticMeshComponent;
class PointLightComponent;

class CullingSubSystem
{
	public:

		DynamicArray<StaticMeshComponent*> GetVisibleStaticMeshesInFrustum(const DynamicArray<StaticMeshComponent*>& InputStaticMeshes, const XMMATRIX& ViewProjMatrix, const bool DoOcclusionTest);
		DynamicArray<PointLightComponent*> GetVisiblePointLightsInFrustum(const DynamicArray<PointLightComponent*>& InputPointLights, const XMMATRIX& ViewProjMatrix);

		float* GetOcclusionBufferData() { return OcclusionBufferData; }
		//float* GetReProjectedOcclusionBufferData() { return ReProjectedOcclusionBufferData; }
		float* GetReProjectedOcclusionBufferData() { return DilatedOcclusionBufferData; }

		void ReProjectOcclusionBuffer(const XMMATRIX& CurrentFrameViewProjMatrix, const uint32_t PreviousFrameIndex);

	private:

		void ExtractFrustumPlanesFromViewProjMatrix(const XMMATRIX& ViewProjMatrix, XMVECTOR* FrustumPlanes);

		bool CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes);
		bool CullSphereVsFrustum(const XMVECTOR& SphereCenter, const float SphereRadius, const XMVECTOR* FrustumPlanes);

		float OcclusionBufferData[256 * 144];
		float ReProjectedOcclusionBufferData[256 * 144];
		float DilatedOcclusionBufferData[256 * 144];

		XMMATRIX PreviousFramesViewProjMatrices[2] = 
		{
			XMMatrixIdentity(),
			XMMatrixIdentity()
		};
};