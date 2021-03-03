#pragma once

#include "../../../Component.h"

class TransformComponent;

class PointLightComponent : public Component
{
	DECLARE_CLASS_WITH_BASE_CLASS(PointLightComponent, Component);

	public:

		virtual void InitComponentDefaultProperties() override;

		virtual void RegisterComponent() override;
		virtual void UnRegisterComponent() override;

		TransformComponent* GetTransformComponent() { return transformComponent; }

		float GetBrightness() const { return Brightness; }
		void SetBrightness(const float NewBrightness) { Brightness = NewBrightness; }

		float GetRadius() const { return Radius; }
		void SetRadius(const float NewRadius) { Radius = NewRadius; }

		XMFLOAT3 GetColor() const { return Color; }
		void SetColor(const XMFLOAT3& NewColor) { Color = NewColor; }

	private:

		float Brightness;
		float Radius;
		XMFLOAT3 Color;

		TransformComponent *transformComponent;
};