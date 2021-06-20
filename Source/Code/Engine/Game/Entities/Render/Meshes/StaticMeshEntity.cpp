// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "StaticMeshEntity.h"

#include "../../../Components/Common/TransformComponent.h"
#include "../../../Components/Common/BoundingBoxComponent.h"
#include "../../../Components/Render/Meshes/StaticMeshComponent.h"

#include <Engine/Engine.h>

#include <MemoryManager/SystemMemoryAllocator.h>

#include <FileSystem/LevelFile.h>

DEFINE_METACLASS_VARIABLE(StaticMeshEntity)

void StaticMeshEntity::InitDefaultProperties()
{
	transformComponent = Component::DynamicCast<TransformComponent>(CreateDefaultComponent(TransformComponent::GetMetaClassStatic()));
	boundingBoxComponent = Component::DynamicCast<BoundingBoxComponent>(CreateDefaultComponent(BoundingBoxComponent::GetMetaClassStatic()));
	staticMeshComponent = Component::DynamicCast<StaticMeshComponent>(CreateDefaultComponent(StaticMeshComponent::GetMetaClassStatic()));
}

void StaticMeshEntity::LoadFromFile(LevelFile& File)
{
	UINT ComponentsCount = File.Read<UINT>();

	for (UINT i = 0; i < ComponentsCount; i++)
	{
		String ComponentClassName = File.Read<String>();

		MetaClass *metaClass = Engine::GetEngine().GetGameFramework().GetMetaClassesTable()[ComponentClassName];

		//void *componentPtr = Engine::GetEngine().GetMemoryManager().AllocateComponent(metaClass);
		void *componentPtr = SystemMemoryAllocator::AllocateMemory(metaClass->GetClassSize());
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
	boundingBoxComponent = GetComponent<BoundingBoxComponent>();
	staticMeshComponent = GetComponent<StaticMeshComponent>();
}