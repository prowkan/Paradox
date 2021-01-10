#include "StaticMeshEntity.h"

#include "../../../MetaClass.h"

#include "../../../Components/Common/TransformComponent.h"
#include "../../../Components/Common/BoundingBoxComponent.h"
#include "../../../Components/Render/Meshes/StaticMeshComponent.h"

MetaClass *StaticMeshEntity::StaticMeshEntityMetaClass;

void StaticMeshEntity::InitDefaultProperties()
{
	transformComponent = (TransformComponent*)CreateDefaultComponent(TransformComponent::GetMetaClassStatic());
	boundingBoxComponent = (BoundingBoxComponent*)CreateDefaultComponent(BoundingBoxComponent::GetMetaClassStatic());
	staticMeshComponent = (StaticMeshComponent*)CreateDefaultComponent(StaticMeshComponent::GetMetaClassStatic());
}