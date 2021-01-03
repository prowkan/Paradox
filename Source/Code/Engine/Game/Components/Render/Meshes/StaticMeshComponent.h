#pragma once

#include "../../../Component.h"

extern MetaClass *StaticMeshComponentMetaClass;

class StaticMeshResource;
class MaterialResource;

class StaticMeshComponent : public Component
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) StaticMeshComponent(); }

		static MetaClass* GetMetaClassStatic() { return StaticMeshComponentMetaClass; }

		virtual void InitComponentDefaultProperties() override;

		StaticMeshResource* GetStaticMesh() { return StaticMesh; }
		void SetStaticMesh(StaticMeshResource* NewStaticMesh) { StaticMesh = NewStaticMesh; }

		MaterialResource* GetMaterial() { return Material; }
		void SetMaterial(MaterialResource* NewMaterial) { Material = NewMaterial; }

	private:

		StaticMeshResource *StaticMesh;
		MaterialResource *Material;

};