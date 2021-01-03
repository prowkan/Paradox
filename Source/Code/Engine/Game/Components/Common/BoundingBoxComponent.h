#pragma once

#include "../../Component.h"

extern MetaClass *BoundingBoxComponentMetaClass;

class BoundingBoxComponent : public Component
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) BoundingBoxComponent(); }

		static MetaClass* GetMetaClassStatic() { return BoundingBoxComponentMetaClass; }

		virtual void InitComponentDefaultProperties() override;

	private:
};