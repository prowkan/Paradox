// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PointLightComponent.h"

#include "../../../Entity.h"

#include "../../Common/TransformComponent.h"

#include <Engine/Engine.h>

DEFINE_METACLASS_VARIABLE(PointLightComponent)

void PointLightComponent::InitComponentDefaultProperties()
{
	transformComponent = Owner->GetComponent<TransformComponent>();
}

void PointLightComponent::RegisterComponent()
{
	Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().RegisterPointLightComponent(this);
}

void PointLightComponent::UnRegisterComponent()
{
}