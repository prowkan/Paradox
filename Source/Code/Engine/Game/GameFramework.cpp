// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GameFramework.h"

#include "MetaClass.h"

#include "Entity.h"
#include "Entities/Render/Meshes/StaticMeshEntity.h"
#include "Entities/Render/Lights/PointLightEntity.h"

#include "Component.h"
#include "Components/Common/TransformComponent.h"
#include "Components/Common/BoundingBoxComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"
#include "Components/Render/Lights/PointLightComponent.h"

void GameFramework::InitFramework()
{
	Entity::InitMetaClass();
	StaticMeshEntity::InitMetaClass();
	PointLightEntity::InitMetaClass();

	Component::InitMetaClass();
	TransformComponent::InitMetaClass();
	BoundingBoxComponent::InitMetaClass();
	StaticMeshComponent::InitMetaClass();
	PointLightComponent::InitMetaClass();

	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("TransformComponent", new ClassProperty());
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties["TransformComponent"]->PropertyType = ClassPropertyType::ComponentReference;
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties["TransformComponent"]->ValueOffset = 64;
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("BoundingBoxComponent", new ClassProperty());
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties["BoundingBoxComponent"]->PropertyType = ClassPropertyType::ComponentReference;
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties["BoundingBoxComponent"]->ValueOffset = 72;
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("StaticMeshComponent", new ClassProperty());
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties["StaticMeshComponent"]->PropertyType = ClassPropertyType::ComponentReference;
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties["StaticMeshComponent"]->ValueOffset = 80;

	PointLightEntity::GetMetaClassStatic()->ClassProperties.emplace("TransformComponent", new ClassProperty());
	PointLightEntity::GetMetaClassStatic()->ClassProperties["TransformComponent"]->PropertyType = ClassPropertyType::ComponentReference;
	PointLightEntity::GetMetaClassStatic()->ClassProperties["TransformComponent"]->ValueOffset = 64;
	PointLightEntity::GetMetaClassStatic()->ClassProperties.emplace("PoingLightComponent", new ClassProperty());
	PointLightEntity::GetMetaClassStatic()->ClassProperties["PoingLightComponent"]->PropertyType = ClassPropertyType::ComponentReference;
	PointLightEntity::GetMetaClassStatic()->ClassProperties["PoingLightComponent"]->ValueOffset = 72;

	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("Location", new ClassProperty());
	TransformComponent::GetMetaClassStatic()->ClassProperties["Location"]->PropertyType = ClassPropertyType::Vector;
	TransformComponent::GetMetaClassStatic()->ClassProperties["Location"]->ValueOffset = 32;
	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("Rotation", new ClassProperty());
	TransformComponent::GetMetaClassStatic()->ClassProperties["Rotation"]->PropertyType = ClassPropertyType::Rotator;
	TransformComponent::GetMetaClassStatic()->ClassProperties["Rotation"]->ValueOffset = 44;
	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("Scale", new ClassProperty());
	TransformComponent::GetMetaClassStatic()->ClassProperties["Scale"]->PropertyType = ClassPropertyType::Vector;
	TransformComponent::GetMetaClassStatic()->ClassProperties["Scale"]->ValueOffset = 56;
	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("PivotPoint", new ClassProperty());
	TransformComponent::GetMetaClassStatic()->ClassProperties["PivotPoint"]->PropertyType = ClassPropertyType::Vector;
	TransformComponent::GetMetaClassStatic()->ClassProperties["PivotPoint"]->ValueOffset = 68;

	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties.emplace("Center", new ClassProperty());
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties["Center"]->PropertyType = ClassPropertyType::Vector;
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties["Center"]->ValueOffset = 32;
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties.emplace("HalfSize", new ClassProperty());
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties["HalfSize"]->PropertyType = ClassPropertyType::Vector;
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties["HalfSize"]->ValueOffset = 44;

	StaticMeshComponent::GetMetaClassStatic()->ClassProperties.emplace("StaticMesh", new ClassProperty());
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties["StaticMesh"]->PropertyType = ClassPropertyType::ResourceReference;
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties["StaticMesh"]->ValueOffset = 32;
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties.emplace("Material", new ClassProperty());
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties["Material"]->PropertyType = ClassPropertyType::ResourceReference;
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties["Material"]->ValueOffset = 40;

	PointLightComponent::GetMetaClassStatic()->ClassProperties.emplace("Brightness", new ClassProperty());
	PointLightComponent::GetMetaClassStatic()->ClassProperties["Brightness"]->PropertyType = ClassPropertyType::Float;
	PointLightComponent::GetMetaClassStatic()->ClassProperties["Brightness"]->ValueOffset = 32;
	PointLightComponent::GetMetaClassStatic()->ClassProperties.emplace("Radius", new ClassProperty());
	PointLightComponent::GetMetaClassStatic()->ClassProperties["Radius"]->PropertyType = ClassPropertyType::Float;
	PointLightComponent::GetMetaClassStatic()->ClassProperties["Radius"]->ValueOffset = 36;
	PointLightComponent::GetMetaClassStatic()->ClassProperties.emplace("Color", new ClassProperty());
	PointLightComponent::GetMetaClassStatic()->ClassProperties["Color"]->PropertyType = ClassPropertyType::Color;
	PointLightComponent::GetMetaClassStatic()->ClassProperties["Color"]->ValueOffset = 40;

	MetaClassesTable.emplace(Entity::GetMetaClassStatic()->GetClassName(), Entity::GetMetaClassStatic());
	MetaClassesTable.emplace(StaticMeshEntity::GetMetaClassStatic()->GetClassName(), StaticMeshEntity::GetMetaClassStatic());
	MetaClassesTable.emplace(PointLightEntity::GetMetaClassStatic()->GetClassName(), PointLightEntity::GetMetaClassStatic());

	MetaClassesTable.emplace(Component::GetMetaClassStatic()->GetClassName(), Component::GetMetaClassStatic());
	MetaClassesTable.emplace(TransformComponent::GetMetaClassStatic()->GetClassName(), TransformComponent::GetMetaClassStatic());
	MetaClassesTable.emplace(BoundingBoxComponent::GetMetaClassStatic()->GetClassName(), BoundingBoxComponent::GetMetaClassStatic());
	MetaClassesTable.emplace(StaticMeshComponent::GetMetaClassStatic()->GetClassName(), StaticMeshComponent::GetMetaClassStatic());
	MetaClassesTable.emplace(PointLightComponent::GetMetaClassStatic()->GetClassName(), PointLightComponent::GetMetaClassStatic());

	camera.InitCamera();
	world.LoadWorld();
}

void GameFramework::ShutdownFramework()
{
	camera.ShutdownCamera();
	world.UnLoadWorld();
}

void GameFramework::TickFramework(float DeltaTime)
{
	camera.TickCamera(DeltaTime);
	world.TickWorld(DeltaTime);
}