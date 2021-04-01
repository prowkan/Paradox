#pragma once

#include "../../../Entity.h"

class TransformComponent;
class BoundingBoxComponent;
class StaticMeshComponent;

class StaticMeshEntity : public Entity
{
	DECLARE_CLASS_WITH_BASE_CLASS(StaticMeshEntity, Entity)

	public:

		virtual void InitDefaultProperties() override;

		virtual void LoadFromFile(HANDLE File) override;

		TransformComponent* GetTransformComponent() { return transformComponent; }
		BoundingBoxComponent* GetBoundingBoxComponent() { return boundingBoxComponent; }
		StaticMeshComponent* GetStaticMeshComponent() { return staticMeshComponent; }

	private:

		TransformComponent *transformComponent;
		BoundingBoxComponent *boundingBoxComponent;
		StaticMeshComponent *staticMeshComponent;
};