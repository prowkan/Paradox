#pragma once

#include "../../../Component.h"

class StaticMeshResource;
class MaterialResource;

class TransformComponent;
class BoundingBoxComponent;

class StaticMeshComponent : public Component
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) StaticMeshComponent(); }

		static MetaClass* GetMetaClassStatic() { return StaticMeshComponentMetaClass; }

		virtual void InitComponentDefaultProperties() override;

		static MetaClass *StaticMeshComponentMetaClass;

		virtual void RegisterComponent() override;
		virtual void UnRegisterComponent() override;

		StaticMeshResource* GetStaticMesh() { return StaticMesh; }
		void SetStaticMesh(StaticMeshResource* NewStaticMesh) { StaticMesh = NewStaticMesh; }

		MaterialResource* GetMaterial() { return Material; }
		void SetMaterial(MaterialResource* NewMaterial) { Material = NewMaterial; }

		TransformComponent* GetTransformComponent() { return transformComponent; }
		BoundingBoxComponent* GetBoundingBoxComponent() { return boundingBoxComponent; }

	private:

		StaticMeshResource *StaticMesh;
		MaterialResource *Material;

		TransformComponent *transformComponent;
		BoundingBoxComponent *boundingBoxComponent;
};