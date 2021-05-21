// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PointLightEntity.h"

#include "../../../Components/Common/TransformComponent.h"
#include "../../../Components/Render/Lights/PointLightComponent.h"

#include <Engine/Engine.h>

#include <MemoryManager/SystemAllocator.h>

#include <FileSystem/LevelFile.h>

DEFINE_METACLASS_VARIABLE(PointLightEntity)

void PointLightEntity::InitDefaultProperties()
{
	transformComponent = Component::DynamicCast<TransformComponent>(CreateDefaultComponent(TransformComponent::GetMetaClassStatic()));
	pointLightComponent = Component::DynamicCast<PointLightComponent>(CreateDefaultComponent(PointLightComponent::GetMetaClassStatic()));
}

void PointLightEntity::LoadFromFile(LevelFile& File)
{
	UINT ComponentsCount = File.Read<UINT>();

	for (UINT i = 0; i < ComponentsCount; i++)
	{
		String ComponentClassName = File.Read<String>();

		MetaClass *metaClass = Engine::GetEngine().GetGameFramework().GetMetaClassesTable()[ComponentClassName];

		//void *componentPtr = Engine::GetEngine().GetMemoryManager().AllocateComponent(metaClass);
		void *componentPtr = SystemAllocator::AllocateMemory(metaClass->GetClassSize());
		metaClass->ObjectConstructorFunc(componentPtr);
		Component *component = (Component*)componentPtr;
		component->SetMetaClass(metaClass);
		component->SetOwner(this);
		//component->InitComponentDefaultProperties();
		component->LoadFromFile(File);
		component->RegisterComponent();

		Components.Add(component);
	}

	transformComponent = GetComponent<TransformComponent>();
	pointLightComponent = GetComponent<PointLightComponent>();
}