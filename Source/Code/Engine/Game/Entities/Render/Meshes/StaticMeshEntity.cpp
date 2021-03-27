// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "StaticMeshEntity.h"

#include "../../../Components/Common/TransformComponent.h"
#include "../../../Components/Common/BoundingBoxComponent.h"
#include "../../../Components/Render/Meshes/StaticMeshComponent.h"

#include <Engine/Engine.h>

DEFINE_METACLASS_VARIABLE(StaticMeshEntity)

void StaticMeshEntity::InitDefaultProperties()
{
	transformComponent = Component::DynamicCast<TransformComponent>(CreateDefaultComponent(TransformComponent::GetMetaClassStatic()));
	boundingBoxComponent = Component::DynamicCast<BoundingBoxComponent>(CreateDefaultComponent(BoundingBoxComponent::GetMetaClassStatic()));
	staticMeshComponent = Component::DynamicCast<StaticMeshComponent>(CreateDefaultComponent(StaticMeshComponent::GetMetaClassStatic()));
}

void StaticMeshEntity::LoadFromFile(HANDLE File)
{
	UINT ComponentsCount;
	BOOL Result = ReadFile(File, &ComponentsCount, sizeof(UINT), NULL, NULL);

	for (UINT i = 0; i < ComponentsCount; i++)
	{
		char ComponentClassName[128];

		Result = ReadFile(File, ComponentClassName, 128, NULL, NULL);

		MetaClass *metaClass = Engine::GetEngine().GetGameFramework().GetMetaClassesTable()[ComponentClassName];

		void *componentPtr = Engine::GetEngine().GetMemoryManager().AllocateComponent(metaClass);
		metaClass->ObjectConstructorFunc(componentPtr);
		Component *component = (Component*)componentPtr;
		component->SetMetaClass(metaClass);
		component->SetOwner(this);
		//component->InitComponentDefaultProperties();
		component->LoadFromFile(File);
		component->RegisterComponent();

		Components.push_back(component);
	}

	transformComponent = GetComponent<TransformComponent>();
	boundingBoxComponent = GetComponent<BoundingBoxComponent>();
	staticMeshComponent = GetComponent<StaticMeshComponent>();
}