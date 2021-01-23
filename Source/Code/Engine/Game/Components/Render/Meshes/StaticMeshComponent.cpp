#include "StaticMeshComponent.h"

#include "../../../Entity.h"

#include "../../Common/TransformComponent.h"
#include "../../Common/BoundingBoxComponent.h"

#include <Engine/Engine.h>

DEFINE_METACLASS_VARIABLE(StaticMeshComponent)

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