// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "StaticMeshComponent.h"

#include "../../../Entity.h"

#include "../../Common/TransformComponent.h"
#include "../../Common/BoundingBoxComponent.h"

#include <Engine/Engine.h>

#include <FileSystem/LevelFile.h>

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

void StaticMeshComponent::LoadFromFile(LevelFile& File)
{
	String StaticMeshResourceName = File.Read<String>();
	String MaterialResourceName = File.Read<String>();

	StaticMesh = Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName);
	Material = Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName);

	transformComponent = Owner->GetComponent<TransformComponent>();
	boundingBoxComponent = Owner->GetComponent<BoundingBoxComponent>();
}