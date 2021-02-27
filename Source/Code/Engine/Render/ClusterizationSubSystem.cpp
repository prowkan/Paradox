// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ClusterizationSubSystem.h"

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

void ClusterizationSubSystem::PreComputeClustersPlanes()
{
	int ClusterIndex = 0;

	XMMATRIX ProjMatrix = XMMatrixPerspectiveFovLH(3.14f / 2.0f, 16.0f / 9.0f, 0.01f, 1000.0f);
	
	for (int z = 0; z < 25; z++)
	{
		float ViewZ = 0.01f * pow(1000.0f / 0.01f, z / 24.0f);

		XMVECTOR PlaneVertices[4];

		PlaneVertices[0] = XMVectorSet(-1.0f * ViewZ / ProjMatrix.m[0][0], 1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);
		PlaneVertices[1] = XMVectorSet(1.0f * ViewZ / ProjMatrix.m[0][0], 1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);
		PlaneVertices[2] = XMVectorSet(-1.0f * ViewZ / ProjMatrix.m[0][0], -1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);
		PlaneVertices[3] = XMVectorSet(1.0f * ViewZ / ProjMatrix.m[0][0], -1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);

		ZPlanes[z] = XMPlaneNormalize(XMPlaneFromPoints(PlaneVertices[0], PlaneVertices[2], PlaneVertices[1]));
	}

	for (int y = 0; y < 19; y++)
	{
		float ViewYNear = 2.0f * ((18 - y) / 18.0f) - 1.0f;
		ViewYNear = ViewYNear * 0.01f / ProjMatrix.m[1][1];

		float ViewYFar = 2.0f * ((18 - y) / 18.0f) - 1.0f;
		ViewYFar = ViewYFar * 1000.0f / ProjMatrix.m[1][1];

		XMVECTOR PlaneVertices[4];

		PlaneVertices[0] = XMVectorSet(-1.0f * 0.01f / ProjMatrix.m[0][0], ViewYNear, 0.01f, 1.0f);
		PlaneVertices[1] = XMVectorSet(1.0f * 0.01f / ProjMatrix.m[0][0], ViewYNear, 0.01f, 1.0f);
		PlaneVertices[2] = XMVectorSet(-1.0f * 1000.0f / ProjMatrix.m[0][0], ViewYFar, 1000.0f, 1.0f);
		PlaneVertices[3] = XMVectorSet(1.0f * 1000.0f / ProjMatrix.m[0][0], ViewYFar, 1000.0f, 1.0f);

		YPlanes[y] = XMPlaneNormalize(XMPlaneFromPoints(PlaneVertices[0], PlaneVertices[1], PlaneVertices[2]));
	}

	for (int x = 0; x < 33; x++)
	{
		float ViewXNear = 2.0f * (x / 32.0f) - 1.0f;
		ViewXNear = ViewXNear * 0.01f / ProjMatrix.m[0][0];

		float ViewXFar = 2.0f * (x / 32.0f) - 1.0f;
		ViewXFar = ViewXFar * 1000.0f / ProjMatrix.m[0][0];

		XMVECTOR PlaneVertices[4];

		PlaneVertices[0] = XMVectorSet(ViewXNear, 1.0f * 0.01f / ProjMatrix.m[1][1], 0.01f, 1.0f);
		PlaneVertices[1] = XMVectorSet(ViewXFar, 1.0f * 1000.0f / ProjMatrix.m[1][1], 1000.0f, 1.0f);
		PlaneVertices[2] = XMVectorSet(ViewXNear, -1.0f * 0.01f / ProjMatrix.m[1][1], 0.01f, 1.0f);
		PlaneVertices[3] = XMVectorSet(ViewXFar, -1.0f * 1000.0f / ProjMatrix.m[1][1], 1000.0f, 1.0f);

		XPlanes[x] = XMPlaneNormalize(XMPlaneFromPoints(PlaneVertices[0], PlaneVertices[1], PlaneVertices[2]));
	}
}

void ClusterizationSubSystem::ClusterizeLights(const vector<PointLightComponent*>& InputPointLights, const XMMATRIX& ViewMatrix)
{
	TotalIndexCount = 0;

	for (int i = 0; i < 32 * 18 * 24; i++)
	{
		LightClustersData[i].Count = 0;
		LightClustersData[i].Offset = 0;
	}

	size_t PointLightsCount = InputPointLights.size();

	for (size_t i = 0; i < PointLightsCount; i++)
	{
		XMFLOAT3 Location = InputPointLights[i]->GetTransformComponent()->GetLocation();
		XMVECTOR SphereCenter = XMVectorSet(Location.x, Location.y, Location.z, 1.0f);
		SphereCenter = XMVector4Transform(SphereCenter, ViewMatrix);
		float SphereRadius = InputPointLights[i]->GetRadius();

		for (int z = 0; z < 24; z++)
		{
			if (XMVectorGetX(XMPlaneDotCoord(ZPlanes[z], SphereCenter)) < -SphereRadius) continue;
			if (XMVectorGetX(XMPlaneDotCoord(ZPlanes[z + 1], SphereCenter)) > SphereRadius) continue;

			for (int y = 0; y < 18; y++)
			{
				if (XMVectorGetX(XMPlaneDotCoord(YPlanes[y], SphereCenter)) < -SphereRadius) continue;
				if (XMVectorGetX(XMPlaneDotCoord(YPlanes[y + 1], SphereCenter)) > SphereRadius) continue;

				for (int x = 0; x < 32; x++)
				{
					if (XMVectorGetX(XMPlaneDotCoord(XPlanes[x], SphereCenter)) < -SphereRadius) continue;
					if (XMVectorGetX(XMPlaneDotCoord(XPlanes[x + 1], SphereCenter)) > SphereRadius) continue;

					int ClusterIndex = z * 32 * 18 + y * 32 + x;

					LocalLightIndices[ClusterIndex][LightClustersData[ClusterIndex].Count] = i;
					LightClustersData[ClusterIndex].Count++;
				}
			}
		}
	}

	int ClusterIndex = 0;

	for (int z = 0; z < 24; z++)
	{
		for (int y = 0; y < 18; y++)
		{
			for (int x = 0; x < 32; x++)
			{
				if (ClusterIndex > 0)
				{
					LightClustersData[ClusterIndex].Offset = LightClustersData[ClusterIndex - 1].Offset + LightClustersData[ClusterIndex - 1].Count;
				}

				for (int i = 0; i < LightClustersData[ClusterIndex].Count; i++)
				{
					LightIndicesData[TotalIndexCount] = LocalLightIndices[ClusterIndex][i];

					TotalIndexCount++;
				}				

				ClusterIndex++;
			}
		}
	}
}