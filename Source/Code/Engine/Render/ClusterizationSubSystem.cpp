// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ClusterizationSubSystem.h"

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

void ClusterizationSubSystem::PreComputeClustersPlanes()
{
	int ClusterIndex = 0;

	XMMATRIX ProjMatrix = XMMatrixPerspectiveFovLH(3.14f / 2.0f, 16.0f / 9.0f, 0.01f, 1000.0f);
	
	/*for (int z = 0; z < 24; z++)
	{
		float zNear = 0.01f * pow(1000.0f / 0.01f, z / 24.0f);
		float zFar = 0.01f * pow(1000.0f / 0.01f, (z + 1) / 24.0f);

		for (int y = 0; y < 18; y++)
		{
			float yTop = 2.0f * ((18 - y) / 18.0f) - 1.0f;
			float yBottom = 2.0f * ((18 - y + 1) / 18.0f) - 1.0f;

			for (int x = 0; x < 32; x++)
			{
				float xLeft = 2.0f * (x / 32.0f) - 1.0f;
				float xRight = 2.0f * ((x + 1) / 32.0f) - 1.0f;

				XMVECTOR ClusterVertices[8];

				ClusterVertices[0] = XMVectorSet(xLeft * zNear / ProjMatrix.m[0][0], yBottom * zNear / ProjMatrix.m[1][1], zNear, 1.0f);
				ClusterVertices[1] = XMVectorSet(xRight * zNear / ProjMatrix.m[0][0], yBottom * zNear / ProjMatrix.m[1][1], zNear, 1.0f);
				ClusterVertices[2] = XMVectorSet(xLeft * zNear / ProjMatrix.m[0][0], yTop * zNear / ProjMatrix.m[1][1], zNear, 1.0f);
				ClusterVertices[3] = XMVectorSet(xRight * zNear / ProjMatrix.m[0][0], yTop * zNear / ProjMatrix.m[1][1], zNear, 1.0f);
				ClusterVertices[4] = XMVectorSet(xLeft * zFar / ProjMatrix.m[0][0], yBottom * zFar / ProjMatrix.m[1][1], zFar, 1.0f);
				ClusterVertices[5] = XMVectorSet(xRight * zFar / ProjMatrix.m[0][0], yBottom * zFar / ProjMatrix.m[1][1], zFar, 1.0f);
				ClusterVertices[6] = XMVectorSet(xLeft * zFar / ProjMatrix.m[0][0], yTop * zFar / ProjMatrix.m[1][1], zFar, 1.0f);
				ClusterVertices[7] = XMVectorSet(xRight * zFar / ProjMatrix.m[0][0], yTop * zFar / ProjMatrix.m[1][1], zFar, 1.0f);

				ClustersPlanes[ClusterIndex][0] = XMPlaneFromPoints(ClusterVertices[0], ClusterVertices[2], ClusterVertices[1]);
				ClustersPlanes[ClusterIndex][1] = XMPlaneFromPoints(ClusterVertices[6], ClusterVertices[4], ClusterVertices[5]);
				ClustersPlanes[ClusterIndex][2] = XMPlaneFromPoints(ClusterVertices[0], ClusterVertices[1], ClusterVertices[5]);
				ClustersPlanes[ClusterIndex][3] = XMPlaneFromPoints(ClusterVertices[6], ClusterVertices[7], ClusterVertices[2]);
				ClustersPlanes[ClusterIndex][4] = XMPlaneFromPoints(ClusterVertices[0], ClusterVertices[4], ClusterVertices[6]);
				ClustersPlanes[ClusterIndex][5] = XMPlaneFromPoints(ClusterVertices[7], ClusterVertices[5], ClusterVertices[3]);

				ClusterIndex++;
			}
		}
	}*/

	for (int z = 0; z < 25; z++)
	{
		float ViewZ = 0.01f * pow(1000.0f / 0.01f, z / 24.0f);

		XMVECTOR PlaneVertices[4];

		PlaneVertices[0] = XMVectorSet(-1.0f * ViewZ / ProjMatrix.m[0][0], 1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);
		PlaneVertices[1] = XMVectorSet(1.0f * ViewZ / ProjMatrix.m[0][0], 1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);
		PlaneVertices[2] = XMVectorSet(-1.0f * ViewZ / ProjMatrix.m[0][0], -1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);
		PlaneVertices[3] = XMVectorSet(1.0f * ViewZ / ProjMatrix.m[0][0], -1.0f * ViewZ / ProjMatrix.m[1][1], ViewZ, 1.0f);

		ZPlanes[z][0] = XMPlaneNormalize(XMPlaneFromPoints(PlaneVertices[0], PlaneVertices[2], PlaneVertices[1]));
		ZPlanes[z][1] = XMVectorNegate(ZPlanes[z][0]);
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

		YPlanes[y][0] = XMPlaneNormalize(XMPlaneFromPoints(PlaneVertices[0], PlaneVertices[1], PlaneVertices[2]));
		YPlanes[y][1] = XMVectorNegate(YPlanes[y][0]);
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

		XPlanes[x][0] = XMPlaneNormalize(XMPlaneFromPoints(PlaneVertices[0], PlaneVertices[1], PlaneVertices[2]));
		XPlanes[x][1] = XMVectorNegate(XPlanes[x][0]);
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
			if (XMVectorGetX(XMPlaneDotCoord(ZPlanes[z][0], SphereCenter)) < -SphereRadius) continue;
			if (XMVectorGetX(XMPlaneDotCoord(ZPlanes[z + 1][1], SphereCenter)) < -SphereRadius) continue;

			for (int y = 0; y < 18; y++)
			{
				if (XMVectorGetX(XMPlaneDotCoord(YPlanes[y][0], SphereCenter)) < -SphereRadius) continue;
				if (XMVectorGetX(XMPlaneDotCoord(YPlanes[y + 1][1], SphereCenter)) < -SphereRadius) continue;

				for (int x = 0; x < 32; x++)
				{
					if (XMVectorGetX(XMPlaneDotCoord(XPlanes[x][0], SphereCenter)) < -SphereRadius) continue;
					if (XMVectorGetX(XMPlaneDotCoord(XPlanes[x + 1][1], SphereCenter)) < -SphereRadius) continue;


					LocalLightIndices[z * 32 * 18 + y * 32 + x][LightClustersData[z * 32 * 18 + y * 32 + x].Count] = i;
					LightClustersData[z * 32 * 18 + y * 32 + x].Count++;
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

				for (int i = 0; i < LightClustersData[z * 32 * 18 + y * 32 + x].Count; i++)
				{
					LightIndicesData[TotalIndexCount] = LocalLightIndices[z * 32 * 18 + y * 32 + x][i];

					TotalIndexCount++;
				}				

				ClusterIndex++;
			}
		}
	}
}