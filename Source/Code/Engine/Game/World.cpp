// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "World.h"

#include "MetaClass.h"
#include "Entity.h"

#include "Entities/Render/Meshes/StaticMeshEntity.h"
#include "Entities/Render/Lights/PointLightEntity.h"

#include "Components/Common/TransformComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"
#include "Components/Render/Lights/PointLightComponent.h"

#include <Containers/COMRCPtr.h>

#include <Engine/Engine.h>

#include <MemoryManager/SystemMemoryAllocator.h>

#include <FileSystem/LevelFile.h>

void World::LoadWorld()
{
	LevelFile MapFile;
	MapFile.OpenFile((const wchar_t*)u"GameContent/Maps/000.dmap");

	UINT EntitiesCount = MapFile.Read<UINT>();

	for (UINT i = 0; i < EntitiesCount; i++)
	{
		String EntityClassName = MapFile.Read<String>();

		MetaClass *metaClass = Engine::GetEngine().GetGameFramework().GetMetaClassesTable()[EntityClassName];

		//void *entityPtr = Engine::GetEngine().GetMemoryManager().AllocateEntity(metaClass);
		void *entityPtr = SystemMemoryAllocator::AllocateMemory(metaClass->GetClassSize());
		metaClass->ObjectConstructorFunc(entityPtr);
		Entity *entity = (Entity*)entityPtr;
		String EntityName = String(metaClass->GetClassName()) + "_" + String((int)metaClass->InstancesCount);
		metaClass->InstancesCount++;
		entity->EntityName = new char[EntityName.GetLength() + 1];
		strcpy((char*)entity->EntityName, EntityName.GetData());
		entity->SetMetaClass(metaClass);
		entity->SetWorld(this);
		entity->LoadFromFile(MapFile);

		Entities.Add(entity);
	}

	MapFile.CloseFile();
}

void World::UnLoadWorld()
{
	Engine::GetEngine().GetResourceManager().DestroyAllResources();
}

Entity* World::SpawnEntity(MetaClass* metaClass)
{
	//void *entityPtr = Engine::GetEngine().GetMemoryManager().AllocateEntity(metaClass);
	void *entityPtr = SystemMemoryAllocator::AllocateMemory(metaClass->GetClassSize());
	metaClass->ObjectConstructorFunc(entityPtr);
	Entity *entity = (Entity*)entityPtr;
	String EntityName = String(metaClass->GetClassName()) + "_" + String((int)metaClass->InstancesCount);
	metaClass->InstancesCount++;
	entity->EntityName = new char[EntityName.GetLength() + 1];
	strcpy((char*)entity->EntityName, EntityName.GetData());
	entity->SetMetaClass(metaClass);
	entity->SetWorld(this);
	entity->InitDefaultProperties();
	Entities.Add(entity);
	return entity;
}

Entity* World::FindEntityByName(const char* EntityName)
{
	for (Entity* entity : Entities)
	{
		if (strcmp(entity->EntityName, EntityName) == 0) return entity;
	}

	return nullptr;
}

void World::TickWorld(float DeltaTime)
{

}