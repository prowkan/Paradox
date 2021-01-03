#include "StaticMeshComponent.h"

#include "../../../MetaClass.h"

MetaClass *StaticMeshComponentMetaClass = new MetaClass(&CallObjectConstructor<StaticMeshComponent>, sizeof(StaticMeshComponent));

void StaticMeshComponent::InitComponentDefaultProperties()
{

}