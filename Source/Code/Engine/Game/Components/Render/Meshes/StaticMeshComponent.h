#pragma once

#include "../../../Component.h"

extern MetaClass *StaticMeshComponentMetaClass;

class StaticMeshComponent : public Component
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) StaticMeshComponent(); }

		static MetaClass* GetMetaClassStatic() { return StaticMeshComponentMetaClass; }

		virtual void InitComponentDefaultProperties() override;

	private:

};