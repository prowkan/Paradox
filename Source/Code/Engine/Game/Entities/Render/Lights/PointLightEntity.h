#pragma once

#include "../../../Entity.h"

class TransformComponent;
class PointLightComponent;

class PointLightEntity : public Entity
{
	DECLARE_CLASS_WITH_BASE_CLASS(PointLightEntity, Entity)

	public:

		virtual void InitDefaultProperties() override;

		virtual void LoadFromFile(LevelFile& File) override;

		TransformComponent* GetTransformComponent() { return transformComponent; }
		PointLightComponent* GetPointLightComponent() { return pointLightComponent; }

	private:

		TransformComponent *transformComponent;
		PointLightComponent *pointLightComponent;
};