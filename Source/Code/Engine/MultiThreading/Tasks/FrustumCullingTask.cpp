// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "FrustumCullingTask.h"

#include <Engine/Engine.h>

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

	if (!DoOcclusionTest) return true;

	XMMATRIX ViewProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewProjMatrix();

	XMMATRIX WVPMatrix = XMMatrixScaling(1.1f, 1.1f, 1.1f) * WorldMatrix * ViewProjMatrix;

	for (int i = 0; i < 8; i++)
	{
		TransformedBoundingBoxVertices[i] = XMVector4Transform(BoundingBoxVertices[i], WVPMatrix);

		if (TransformedBoundingBoxVertices[i].vector4_f32[3] < 0.01f) return true;

		TransformedBoundingBoxVertices[i].vector4_f32[0] /= TransformedBoundingBoxVertices[i].vector4_f32[3];
		TransformedBoundingBoxVertices[i].vector4_f32[1] /= TransformedBoundingBoxVertices[i].vector4_f32[3];
		TransformedBoundingBoxVertices[i].vector4_f32[2] /= TransformedBoundingBoxVertices[i].vector4_f32[3];

		TransformedBoundingBoxVertices[i].vector4_f32[0] = 128.0f * TransformedBoundingBoxVertices[i].vector4_f32[0] + 128.0f;
		TransformedBoundingBoxVertices[i].vector4_f32[1] = -72.0f * TransformedBoundingBoxVertices[i].vector4_f32[1] + 72.0f;
	}

	float *OcclusionBufferData = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetOcclusionBufferData();

	static WORD BBIndices[36] =
	{
		5, 4, 7, 7, 4, 6,
		0, 1, 2, 2, 1, 3,
		4, 0, 6, 6, 0, 2,
		1, 5, 3, 3, 5, 7,
		1, 0, 5, 5, 0, 4,
		2, 3, 6, 6, 3, 7
	};

	for (int i = 0; i < 12; i++)
	{
		XMFLOAT3 TrianglePoints[3] =
		{
			XMFLOAT3(TransformedBoundingBoxVertices[BBIndices[3 * i]].vector4_f32[0], TransformedBoundingBoxVertices[BBIndices[3 * i]].vector4_f32[1], TransformedBoundingBoxVertices[BBIndices[3 * i]].vector4_f32[2]),
			XMFLOAT3(TransformedBoundingBoxVertices[BBIndices[3 * i + 1]].vector4_f32[0], TransformedBoundingBoxVertices[BBIndices[3 * i + 1]].vector4_f32[1], TransformedBoundingBoxVertices[BBIndices[3 * i + 1]].vector4_f32[2]),
			XMFLOAT3(TransformedBoundingBoxVertices[BBIndices[3 * i + 2]].vector4_f32[0], TransformedBoundingBoxVertices[BBIndices[3 * i + 2]].vector4_f32[1], TransformedBoundingBoxVertices[BBIndices[3 * i + 2]].vector4_f32[2])
		};

		float TriangleArea = (TrianglePoints[1].x - TrianglePoints[0].x) * (TrianglePoints[2].y - TrianglePoints[0].y) - (TrianglePoints[2].x - TrianglePoints[0].x) * (TrianglePoints[1].y - TrianglePoints[0].y);

		if (TriangleArea <= 0.0f) continue;

		float MinXf = min(min(TrianglePoints[0].x, TrianglePoints[1].x), TrianglePoints[2].x);
		float MaxXf = max(max(TrianglePoints[0].x, TrianglePoints[1].x), TrianglePoints[2].x);
		float MinYf = min(min(TrianglePoints[0].y, TrianglePoints[1].y), TrianglePoints[2].y);
		float MaxYf = max(max(TrianglePoints[0].y, TrianglePoints[1].y), TrianglePoints[2].y);

		int MinX = floorf(MinXf);
		int MaxX = ceilf(MaxXf);
		int MinY = floorf(MinYf);
		int MaxY = ceilf(MaxYf);

#define clamp(x, a, b) x = (x < a) ? (a) : ((x > b) ? (b) : (x))

		MinX = clamp(MinX, 0, 255);
		MaxX = clamp(MaxX, 0, 255);
		MinY = clamp(MinY, 0, 143);
		MaxY = clamp(MaxY, 0, 143);

		float StartU = ((TrianglePoints[1].x - MinX) * (TrianglePoints[2].y - MinY) - (TrianglePoints[2].x - MinX) * (TrianglePoints[1].y - MinY)) / TriangleArea;
		float StartV = ((TrianglePoints[2].x - MinX) * (TrianglePoints[0].y - MinY) - (TrianglePoints[0].x - MinX) * (TrianglePoints[2].y - MinY)) / TriangleArea;

		float U, V, W;

		float dUdx = (TrianglePoints[1].y - TrianglePoints[2].y) / TriangleArea;
		float dUdy = (TrianglePoints[2].x - TrianglePoints[1].x) / TriangleArea;
		float dVdx = (TrianglePoints[2].y - TrianglePoints[0].y) / TriangleArea;
		float dVdy = (TrianglePoints[0].x - TrianglePoints[2].x) / TriangleArea;

		float RowU = StartU;
		float RowV = StartV;

		for (int y = MinY; y <= MaxY; y++)
		{
			U = RowU;
			V = RowV;

			for (int x = MinX; x <= MaxX; x++)
			{
				W = 1.0f - U - V;

				if (U < 0.0f || V < 0.0f || W < 0.0f)
				{
					U += dUdx;
					V += dVdx;

					continue;
				}

				float TriangleDepth = U * TrianglePoints[0].z + V * TrianglePoints[1].z + W * TrianglePoints[2].z;
				float TexelDepth = OcclusionBufferData[y * 256 + x];

				if (TriangleDepth >= TexelDepth) return true;

				U += dUdx;
				V += dVdx;
			}

			RowU += dUdy;
			RowV += dVdy;
		}
	}

	return false;
}