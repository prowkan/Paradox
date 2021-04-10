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

#ifdef _DEBUG
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("BoundingBoxComponent", new ClassProperty(ClassPropertyType::ComponentReference, 72));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("StaticMeshComponent", new ClassProperty(ClassPropertyType::ComponentReference, 80));
	
	PointLightEntity::GetMetaClassStatic()->ClassProperties.emplace("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
	PointLightEntity::GetMetaClassStatic()->ClassProperties.emplace("PoingLightComponent", new ClassProperty(ClassPropertyType::ComponentReference, 72));
#else
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 56));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("BoundingBoxComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.emplace("StaticMeshComponent", new ClassProperty(ClassPropertyType::ComponentReference, 72));

	PointLightEntity::GetMetaClassStatic()->ClassProperties.emplace("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 56));
	PointLightEntity::GetMetaClassStatic()->ClassProperties.emplace("PoingLightComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
#endif

	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("Location", new ClassProperty(ClassPropertyType::Vector, 32));
	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("Rotation", new ClassProperty(ClassPropertyType::Rotator, 44));
	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("Scale", new ClassProperty(ClassPropertyType::Vector, 56));
	TransformComponent::GetMetaClassStatic()->ClassProperties.emplace("PivotPoint", new ClassProperty(ClassPropertyType::Vector, 68));
	
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties.emplace("Center", new ClassProperty(ClassPropertyType::Vector, 32));
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties.emplace("HalfSize", new ClassProperty(ClassPropertyType::Vector, 44));
	
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties.emplace("StaticMesh", new ClassProperty(ClassPropertyType::ResourceReference, 32));
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties.emplace("Material", new ClassProperty(ClassPropertyType::ResourceReference, 40));
	
	PointLightComponent::GetMetaClassStatic()->ClassProperties.emplace("Brightness", new ClassProperty(ClassPropertyType::Float, 32));
	PointLightComponent::GetMetaClassStatic()->ClassProperties.emplace("Radius", new ClassProperty(ClassPropertyType::Float, 36));
	PointLightComponent::GetMetaClassStatic()->ClassProperties.emplace("Color", new ClassProperty(ClassPropertyType::Color, 40));
	
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