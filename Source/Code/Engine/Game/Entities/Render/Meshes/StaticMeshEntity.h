#pragma once

#include "../../../Entity.h"

class TransformComponent;
class BoundingBoxComponent;
class StaticMeshComponent;

class StaticMeshEntity : public Entity
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) StaticMeshEntity(); }

		static MetaClass* GetMetaClassStatic() { return StaticMeshEntityMetaClass; }

		static MetaClass *StaticMeshEntityMetaClass;

		virtual void InitDefaultProperties() override;

		TransformComponent* GetTransformComponent() { return transformComponent; }
		BoundingBoxComponent* GetBoundingBoxComponent() { return boundingBoxComponent; }
		StaticMeshComponent* GetStaticMeshComponent() { return staticMeshComponent; }

	private:

		TransformComponent *transformComponent;
		BoundingBoxComponent *boundingBoxComponent;
		StaticMeshComponent *staticMeshComponent;
};