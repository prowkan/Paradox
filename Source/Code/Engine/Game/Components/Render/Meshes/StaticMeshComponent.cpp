#include "StaticMeshComponent.h"

#include "../../../MetaClass.h"
#include "../../../GameObject.h"

#include "../../Common/TransformComponent.h"
#include "../../Common/BoundingBoxComponent.h"

MetaClass *StaticMeshComponentMetaClass = new MetaClass(&CallObjectConstructor<StaticMeshComponent>, sizeof(StaticMeshComponent), "StaticMeshComponent");

void StaticMeshComponent::InitComponentDefaultProperties()
{
	transformComponent = Owner->GetComponent<TransformComponent>();
	boundingBoxComponent = Owner->GetComponent<BoundingBoxComponent>();
}