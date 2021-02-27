#pragma once

class PointLightComponent;

struct LightCluster
{
	uint32_t Offset;
	uint32_t Count;
};

class ClusterizationSubSystem
{
	public:

		LightCluster* GetLightClustersData() { return LightClustersData; }
		uint16_t* GetLightIndicesData() { return LightIndicesData; }
		uint32_t GetTotalIndexCount() { return TotalIndexCount; }

		void PreComputeClustersPlanes();

		void ClusterizeLights(const vector<PointLightComponent*>& InputPointLights, const XMMATRIX& ViewMatrix);

	private:

		LightCluster LightClustersData[32 * 18 * 24];
		uint16_t LightIndicesData[256 * 32 * 18 * 24];
		uint32_t TotalIndexCount;

		XMVECTOR XPlanes[33], YPlanes[19], ZPlanes[25];

		uint16_t LocalLightIndices[32 * 18 * 24][256];
};