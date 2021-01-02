#include "BoundingBoxComponent.h"

#include "../../MetaClass.h"

MetaClass *BoundingBoxComponentMetaClass = new MetaClass(&CallObjectConstructor<BoundingBoxComponent>, sizeof(BoundingBoxComponent));

void BoundingBoxComponent::InitComponentDefaultProperties()
{
}