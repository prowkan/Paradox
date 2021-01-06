#include "StaticMeshObject.h"

#include "../../../MetaClass.h"

#include "../../../Components/Common/TransformComponent.h"
#include "../../../Components/Common/BoundingBoxComponent.h"
#include "../../../Components/Render/Meshes/StaticMeshComponent.h"

MetaClass *StaticMeshObject::StaticMeshObjectMetaClass;

void StaticMeshObject::InitDefaultProperties()
{
	transformComponent = (TransformComponent*)CreateDefaultComponent(TransformComponent::GetMetaClassStatic());
	boundingBoxComponent = (BoundingBoxComponent*)CreateDefaultComponent(BoundingBoxComponent::GetMetaClassStatic());
	staticMeshComponent = (StaticMeshComponent*)CreateDefaultComponent(StaticMeshComponent::GetMetaClassStatic());
}