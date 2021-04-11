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
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.Insert("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.Insert("BoundingBoxComponent", new ClassProperty(ClassPropertyType::ComponentReference, 72));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.Insert("StaticMeshComponent", new ClassProperty(ClassPropertyType::ComponentReference, 80));
	
	PointLightEntity::GetMetaClassStatic()->ClassProperties.Insert("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
	PointLightEntity::GetMetaClassStatic()->ClassProperties.Insert("PoingLightComponent", new ClassProperty(ClassPropertyType::ComponentReference, 72));
#else
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.Insert("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 56));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.Insert("BoundingBoxComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
	StaticMeshEntity::GetMetaClassStatic()->ClassProperties.Insert("StaticMeshComponent", new ClassProperty(ClassPropertyType::ComponentReference, 72));

	PointLightEntity::GetMetaClassStatic()->ClassProperties.Insert("TransformComponent", new ClassProperty(ClassPropertyType::ComponentReference, 56));
	PointLightEntity::GetMetaClassStatic()->ClassProperties.Insert("PoingLightComponent", new ClassProperty(ClassPropertyType::ComponentReference, 64));
#endif

	TransformComponent::GetMetaClassStatic()->ClassProperties.Insert("Location", new ClassProperty(ClassPropertyType::Vector, 32));
	TransformComponent::GetMetaClassStatic()->ClassProperties.Insert("Rotation", new ClassProperty(ClassPropertyType::Rotator, 44));
	TransformComponent::GetMetaClassStatic()->ClassProperties.Insert("Scale", new ClassProperty(ClassPropertyType::Vector, 56));
	TransformComponent::GetMetaClassStatic()->ClassProperties.Insert("PivotPoint", new ClassProperty(ClassPropertyType::Vector, 68));
	
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties.Insert("Center", new ClassProperty(ClassPropertyType::Vector, 32));
	BoundingBoxComponent::GetMetaClassStatic()->ClassProperties.Insert("HalfSize", new ClassProperty(ClassPropertyType::Vector, 44));
	
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties.Insert("StaticMesh", new ClassProperty(ClassPropertyType::ResourceReference, 32));
	StaticMeshComponent::GetMetaClassStatic()->ClassProperties.Insert("Material", new ClassProperty(ClassPropertyType::ResourceReference, 40));
	
	PointLightComponent::GetMetaClassStatic()->ClassProperties.Insert("Brightness", new ClassProperty(ClassPropertyType::Float, 32));
	PointLightComponent::GetMetaClassStatic()->ClassProperties.Insert("Radius", new ClassProperty(ClassPropertyType::Float, 36));
	PointLightComponent::GetMetaClassStatic()->ClassProperties.Insert("Color", new ClassProperty(ClassPropertyType::Color, 40));
	
	MetaClassesTable.Insert(Entity::GetMetaClassStatic()->GetClassName(), Entity::GetMetaClassStatic());
	MetaClassesTable.Insert(StaticMeshEntity::GetMetaClassStatic()->GetClassName(), StaticMeshEntity::GetMetaClassStatic());
	MetaClassesTable.Insert(PointLightEntity::GetMetaClassStatic()->GetClassName(), PointLightEntity::GetMetaClassStatic());

	MetaClassesTable.Insert(Component::GetMetaClassStatic()->GetClassName(), Component::GetMetaClassStatic());
	MetaClassesTable.Insert(TransformComponent::GetMetaClassStatic()->GetClassName(), TransformComponent::GetMetaClassStatic());
	MetaClassesTable.Insert(BoundingBoxComponent::GetMetaClassStatic()->GetClassName(), BoundingBoxComponent::GetMetaClassStatic());
	MetaClassesTable.Insert(StaticMeshComponent::GetMetaClassStatic()->GetClassName(), StaticMeshComponent::GetMetaClassStatic());
	MetaClassesTable.Insert(PointLightComponent::GetMetaClassStatic()->GetClassName(), PointLightComponent::GetMetaClassStatic());

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