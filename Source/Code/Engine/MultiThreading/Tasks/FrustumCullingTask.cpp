#include "FrustumCullingTask.h"

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

void FrustumCullingTask::Execute(const UINT ThreadID)
{
	OPTICK_EVENT("Frustum Culling Task")

	for (size_t i = Begin; i < End; i++)
	{
		XMMATRIX WorldMatrix = InputStaticMeshesArray[i]->GetTransformComponent()->GetTransformMatrix();

		BoundingBoxComponent *boundingBoxComponent = InputStaticMeshesArray[i]->GetBoundingBoxComponent();

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

		if (CullBoxVsFrustum(BoundingBoxVertices, WorldMatrix, FrustumPlanes)) OutputStaticMeshesArray.push_back(InputStaticMeshesArray[i]);
	}
}

bool FrustumCullingTask::CullBoxVsFrustum(const XMVECTOR* BoundingBoxVertices, const XMMATRIX& WorldMatrix, const XMVECTOR* FrustumPlanes)
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