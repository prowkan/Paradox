#pragma once

#include "../../Component.h"

class BoundingBoxComponent : public Component
{
	DECLARE_CLASS_WITH_BASE_CLASS(BoundingBoxComponent, Component)

	public:

		virtual void InitComponentDefaultProperties() override;

		XMFLOAT3 GetCenter() { return Center; }
		XMFLOAT3 GetHalfSize() { return HalfSize; }

	private:

		XMFLOAT3 Center;
		XMFLOAT3 HalfSize;
};