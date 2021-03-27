#pragma once

#include "../../../Component.h"

class StaticMeshResource;
class MaterialResource;

class TransformComponent;
class BoundingBoxComponent;

class StaticMeshComponent : public Component
{
	DECLARE_CLASS_WITH_BASE_CLASS(StaticMeshComponent, Component)

	public:

		virtual void InitComponentDefaultProperties() override;

		virtual void RegisterComponent() override;
		virtual void UnRegisterComponent() override;

		virtual void LoadFromFile(HANDLE File) override;

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