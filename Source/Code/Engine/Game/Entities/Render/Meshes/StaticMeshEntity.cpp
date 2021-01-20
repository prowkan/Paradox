#include "StaticMeshEntity.h"

#include "../../../MetaClass.h"

#include "../../../Components/Common/TransformComponent.h"
#include "../../../Components/Common/BoundingBoxComponent.h"
#include "../../../Components/Render/Meshes/StaticMeshComponent.h"

DEFINE_METACLASS_VARIABLE(StaticMeshEntity)

void StaticMeshEntity::InitDefaultProperties()
{
	transformComponent = Component::DynamicCast<TransformComponent>(CreateDefaultComponent(TransformComponent::GetMetaClassStatic()));
	boundingBoxComponent = Component::DynamicCast<BoundingBoxComponent>(CreateDefaultComponent(BoundingBoxComponent::GetMetaClassStatic()));
	staticMeshComponent = Component::DynamicCast<StaticMeshComponent>(CreateDefaultComponent(StaticMeshComponent::GetMetaClassStatic()));
}