#pragma once

#include <Containers/DynamicArray.h>
#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>

#include "../../../Component.h"

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

		virtual void LoadFromFile(LevelFile& File) override;

		StaticMeshResource* GetStaticMesh() { return StaticMesh; }
		void SetStaticMesh(StaticMeshResource* NewStaticMesh) { StaticMesh = NewStaticMesh; Materials.Resize(StaticMesh->GetTotalElementsCount()); };

		MaterialResource* GetMaterial(const UINT ElementIndex) { return Materials[ElementIndex]; }
		void SetMaterial(MaterialResource* NewMaterial, const UINT ElementIndex) { Materials[ElementIndex] = NewMaterial; }

		TransformComponent* GetTransformComponent() { return transformComponent; }
		BoundingBoxComponent* GetBoundingBoxComponent() { return boundingBoxComponent; }

	private:

		StaticMeshResource *StaticMesh;
		DynamicArray<MaterialResource*> Materials;

		TransformComponent *transformComponent;
		BoundingBoxComponent *boundingBoxComponent;
};