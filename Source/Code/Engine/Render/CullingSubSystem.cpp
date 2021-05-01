// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "CullingSubSystem.h"

#include <Engine/Engine.h>

#include <MultiThreading/Tasks/FrustumCullingTask.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

DynamicArray<StaticMeshComponent*> CullingSubSystem::GetVisibleStaticMeshesInFrustum(const DynamicArray<StaticMeshComponent*>& InputStaticMeshes, const XMMATRIX& ViewProjMatrix, const bool DoOcclusionTest)
{
	OPTICK_EVENT("Frustum Culling")

	DynamicArray<StaticMeshComponent*> OutputStaticMeshes;

	XMVECTOR FrustumPlanes[6];

	ExtractFrustumPlanesFromViewProjMatrix(ViewProjMatrix, FrustumPlanes);

	BYTE FrustumCullingTasksStorage[20 * sizeof(FrustumCullingTask)];
	FrustumCullingTask *FrustumCullingTasks = (FrustumCullingTask*)FrustumCullingTasksStorage;

	for (UINT i = 0; i < 20; i++)
	{
		new (&FrustumCullingTasks[i]) FrustumCullingTask(InputStaticMeshes, FrustumPlanes, i * 1000, (i + 1) * 1000, DoOcclusionTest);

		Engine::GetEngine().GetMultiThreadingSystem().AddTask(&FrustumCullingTasks[i]);
	}

	/*for (int i = 0; i < InputStaticMeshes.size(); i++)
	{
		XMMATRIX WorldMatrix = InputStaticMeshes[i]->GetTransformComponent()->GetTransformMatrix();

		BoundingBoxComponent *boundingBoxComponent = InputStaticMeshes[i]->GetBoundingBoxComponent();

		XMFLOAT3 BBCenter = boundingBoxComponent->GetCenter();
		XMFLOAT3 BBHalfSize = boundingBoxComponent->GetHalfSize();

		XMVECTOR BoundingBoxVertices[8];
		BoundingBoxVertices[0] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
		BoundingBoxVertices[1] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
		BoundingBoxVertices[2] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
		BoundingBoxVertices[3] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z + BBHalfSize.z, 1.0f);
		BoundingBoxVertices[4] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);
		BoundingBoxVertices[5] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y + BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);
		BoundingBoxVertices[6] = XMVectorSet(BBCenter.x + BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);
		BoundingBoxVertices[7] = XMVectorSet(BBCenter.x - BBHalfSize.x, BBCenter.y - BBHalfSize.y, BBCenter.z - BBHalfSize.z, 1.0f);

		if (CullBoxVsFrustum(BoundingBoxVertices, WorldMatrix, FrustumPlanes)) OutputStaticMeshes.push_back(InputStaticMeshes[i]);
	}*/

	for (UINT i = 0; i < 20; i++)
	{
		FrustumCullingTasks[i].WaitForFinish();
		DynamicArray<StaticMeshComponent*>& LocalTaskResult = FrustumCullingTasks[i].GetOutputData();
		OutputStaticMeshes.Append(LocalTaskResult);
		FrustumCullingTasks[i].~FrustumCullingTask();
	}

	return OutputStaticMeshes;
}


DynamicArray<PointLightComponent*> CullingSubSystem::GetVisiblePointLightsInFrustum(const DynamicArray<PointLightComponent*>& InputPointLights, const XMMATRIX& ViewProjMatrix)
{
	DynamicArray<PointLightComponent*> OutputPointLights;

	XMVECTOR FrustumPlanes[6];

	ExtractFrustumPlanesFromViewProjMatrix(ViewProjMatrix, FrustumPlanes);

	for (size_t i = 0; i < InputPointLights.GetLength(); i++)
	{
		XMFLOAT3 Location = InputPointLights[i]->GetTransformComponent()->GetLocation();
		XMVECTOR SphereCenter = XMVectorSet(Location.x, Location.y, Location.z, 1.0f);
		float SphereRadius = InputPointLights[i]->GetRadius();		

		if (CullSphereVsFrustum(SphereCenter, SphereRadius, FrustumPlanes)) OutputPointLights.Add(InputPointLights[i]);
	}

	return OutputPointLights;
}

void CullingSubSystem::ExtractFrustumPlanesFromViewProjMatrix(const XMMATRIX& ViewProjMatrix, XMVECTOR* FrustumPlanes)
{
	XMFLOAT4X4 ViewProjMatrixF44;

	XMStoreFloat4x4(&ViewProjMatrixF44, ViewProjMatrix);

	FrustumPlanes[0] = XMPlaneNormalize(XMVectorSet(ViewProjMatrixF44._14 + ViewProjMatrixF44._11, ViewProjMatrixF44._24 + ViewProjMatrixF44._21, ViewProjMatrixF44._34 + ViewProjMatrixF44._31, ViewProjMatrixF44._44 + ViewProjMatrixF44._41));
	FrustumPlanes[1] = XMPlaneNormalize(XMVectorSet(ViewProjMatrixF44._14 - ViewProjMatrixF44._11, ViewProjMatrixF44._24 - ViewProjMatrixF44._21, ViewProjMatrixF44._34 - ViewProjMatrixF44._31, ViewProjMatrixF44._44 - ViewProjMatrixF44._41));
	FrustumPlanes[2] = XMPlaneNormalize(XMVectorSet(ViewProjMatrixF44._14 + ViewProjMatrixF44._12, ViewProjMatrixF44._24 + ViewProjMatrixF44._22, ViewProjMatrixF44._34 + ViewProjMatrixF44._32, ViewProjMatrixF44._44 + ViewProjMatrixF44._42));
	FrustumPlanes[3] = XMPlaneNormalize(XMVectorSet(ViewProjMatrixF44._14 - ViewProjMatrixF44._12, ViewProjMatrixF44._24 - ViewProjMatrixF44._22, ViewProjMatrixF44._34 - ViewProjMatrixF44._32, ViewProjMatrixF44._44 - ViewProjMatrixF44._42));
	FrustumPlanes[4] = XMPlaneNormalize(XMVectorSet(ViewProjMatrixF44._13, ViewProjMatrixF44._23, ViewProjMatrixF44._33, ViewProjMatrixF44._43));
	FrustumPlanes[5] = XMPlaneNormalize(XMVectorSet(ViewProjMatrixF44._14 - ViewProjMatrixF44._13, ViewProjMatrixF44._24 - ViewProjMatrixF44._23, ViewProjMatrixF44._34 - ViewProjMatrixF44._33, ViewProjMatrixF44._44 - ViewProjMatrixF44._43));
}

bool CullingSubSystem::CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes)
{
	XMVECTOR TransformedBoundingBoxVertices[8];

	for (int i = 0; i < 8; i++)
	{
		TransformedBoundingBoxVertices[i] = XMVector4Transform(BoundingBoxVertices[i], WorldMatrix);
	}

	for (int i = 0; i < 6; i++)
	{
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[0])) > 0.0f) continue;
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[1])) > 0.0f) continue;
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[2])) > 0.0f) continue;
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[3])) > 0.0f) continue;
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[4])) > 0.0f) continue;
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[5])) > 0.0f) continue;
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[6])) > 0.0f) continue;
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], TransformedBoundingBoxVertices[7])) > 0.0f) continue;

		return false;
	}

	return true;
}

bool CullingSubSystem::CullSphereVsFrustum(const XMVECTOR& SphereCenter, const float SphereRadius, const XMVECTOR* FrustumPlanes)
{
	for (int i = 0; i < 6; i++)
	{
		if (XMVectorGetX(XMPlaneDotCoord(FrustumPlanes[i], SphereCenter)) < -SphereRadius) return false;
	}

	return true;
}