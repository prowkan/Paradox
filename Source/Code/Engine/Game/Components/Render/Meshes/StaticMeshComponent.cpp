// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

void StaticMeshComponent::LoadFromFile(HANDLE File)
{
	BOOL Result;
	
	char StaticMeshResourceName[128];
	char MaterialResourceName[128];

	Result = ReadFile(File, StaticMeshResourceName, 128, NULL, NULL);
	Result = ReadFile(File, MaterialResourceName, 128, NULL, NULL);

	StaticMesh = Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName);
	Material = Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName);

	transformComponent = Owner->GetComponent<TransformComponent>();
	boundingBoxComponent = Owner->GetComponent<BoundingBoxComponent>();
}