// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PointLightEntity.h"

#include "../../../Components/Common/TransformComponent.h"
#include "../../../Components/Render/Lights/PointLightComponent.h"

DEFINE_METACLASS_VARIABLE(PointLightEntity)

void PointLightEntity::InitDefaultProperties()
{
	transformComponent = Component::DynamicCast<TransformComponent>(CreateDefaultComponent(TransformComponent::GetMetaClassStatic()));
	pointLightComponent = Component::DynamicCast<PointLightComponent>(CreateDefaultComponent(PointLightComponent::GetMetaClassStatic()));
}