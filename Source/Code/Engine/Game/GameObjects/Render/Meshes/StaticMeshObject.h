#pragma once

#include "../../../GameObject.h"

class TransformComponent;
class BoundingBoxComponent;
class StaticMeshComponent;

class StaticMeshObject : public GameObject
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) StaticMeshObject(); }

		static MetaClass* GetMetaClassStatic() { return StaticMeshObjectMetaClass; }

		static MetaClass *StaticMeshObjectMetaClass;

		virtual void InitDefaultProperties() override;

		TransformComponent* GetTransformComponent() { return transformComponent; }
		BoundingBoxComponent* GetBoundingBoxComponent() { return boundingBoxComponent; }
		StaticMeshComponent* GetStaticMeshComponent() { return staticMeshComponent; }

	private:

		TransformComponent *transformComponent;
		BoundingBoxComponent *boundingBoxComponent;
		StaticMeshComponent *staticMeshComponent;
};