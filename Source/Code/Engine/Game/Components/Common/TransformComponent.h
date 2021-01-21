#pragma once

#include "../../Component.h"

class TransformComponent : public Component
{
	DECLARE_CLASS_WITH_BASE_CLASS(TransformComponent, Component)

	public:

		virtual void InitComponentDefaultProperties() override;

		XMFLOAT3 GetLocation() { return Location; }
		void SetLocation(const XMFLOAT3& NewLocation) { Location = NewLocation; }

		XMFLOAT3 GetRotation() { return Rotation; }
		void SetRotation(const XMFLOAT3& NewRotation) { Rotation = NewRotation; }

		XMFLOAT3 GetScale() { return Scale; }
		void SetScale(const XMFLOAT3& NewScale) { Scale = NewScale; }

		XMFLOAT3 GetPivotPoint() { return PivotPoint; }
		void SetPivotPoint(const XMFLOAT3& NewPivotPoint) { PivotPoint = NewPivotPoint; }

		XMMATRIX GetTransformMatrix() { return XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z) * XMMatrixScaling(Scale.x, Scale.y, Scale.z) * XMMatrixTranslation(Location.x, Location.y, Location.z); }

	private:

		XMFLOAT3 Location;
		XMFLOAT3 Rotation;
		XMFLOAT3 Scale;
		XMFLOAT3 PivotPoint;
};