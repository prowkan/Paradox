#include "StaticMeshComponent.h"

#include "../../../MetaClass.h"
#include "../../../GameObject.h"

#include "../../Common/TransformComponent.h"
#include "../../Common/BoundingBoxComponent.h"

#include <Engine/Engine.h>

MetaClass *StaticMeshComponentMetaClass = new MetaClass(&CallObjectConstructor<StaticMeshComponent>, sizeof(StaticMeshComponent), "StaticMeshComponent");

void StaticMeshComponent::InitComponentDefaultProperties()
{
	transformComponent = Owner->GetComponent<TransformComponent>();
	boundingBoxComponent = Owner->GetComponent<BoundingBoxComponent>();
}

void StaticMeshComponent::RegisterComponent()
{
	Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().RegisterStaticMeshComponent(this);
}

void StaticMeshComponent::UnRegisterComponent()
{
}