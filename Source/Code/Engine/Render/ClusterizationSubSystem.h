#pragma once

#include <Containers/DynamicArray.h>

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

		void ClusterizeLights(const DynamicArray<PointLightComponent*>& InputPointLights, const XMMATRIX& ViewMatrix);

		static const UINT CLUSTERS_COUNT_X = 32;
		static const UINT CLUSTERS_COUNT_Y = 18;
		static const UINT CLUSTERS_COUNT_Z = 24;

		static const UINT MAX_LIGHTS_PER_CLUSTER = 256;

	private:

		LightCluster LightClustersData[CLUSTERS_COUNT_X * CLUSTERS_COUNT_Y * CLUSTERS_COUNT_Z];
		uint16_t LightIndicesData[MAX_LIGHTS_PER_CLUSTER * CLUSTERS_COUNT_X * CLUSTERS_COUNT_Y * CLUSTERS_COUNT_Z];
		uint32_t TotalIndexCount;

		XMVECTOR XPlanes[CLUSTERS_COUNT_X + 1], YPlanes[CLUSTERS_COUNT_Y + 1], ZPlanes[CLUSTERS_COUNT_Z + 1];

		uint16_t LocalLightIndices[CLUSTERS_COUNT_X * CLUSTERS_COUNT_Y * CLUSTERS_COUNT_Z][MAX_LIGHTS_PER_CLUSTER];
};