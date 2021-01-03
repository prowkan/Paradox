#pragma once

#include "../../Component.h"

extern MetaClass *BoundingBoxComponentMetaClass;

class BoundingBoxComponent : public Component
{
	public:

		static void StaticConstructor(void* Pointer) { new (Pointer) BoundingBoxComponent(); }

		static MetaClass* GetMetaClassStatic() { return BoundingBoxComponentMetaClass; }

		virtual void InitComponentDefaultProperties() override;

		XMFLOAT3 GetCenter() { return Center; }
		XMFLOAT3 GetHalfSize() { return HalfSize; }

	private:

		XMFLOAT3 Center;
		XMFLOAT3 HalfSize;
};