#include "BoundingBoxComponent.h"

#include "../../MetaClass.h"

MetaClass *BoundingBoxComponentMetaClass;

void BoundingBoxComponent::InitComponentDefaultProperties()
{
	Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	HalfSize = XMFLOAT3(1.0f, 1.0f, 1.0f);
}