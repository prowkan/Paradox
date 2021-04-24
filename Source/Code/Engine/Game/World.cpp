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

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>

#include <MemoryManager/SystemAllocator.h>

void World::LoadWorld()
{
	struct StaticMeshFileHeader
	{
		UINT VertexCount;
		UINT IndexCount;
	};

	struct TextureFileHeader
	{
		UINT Width;
		UINT Height;
		UINT MIPLevels;
		BOOL SRGB;
		BOOL Compressed;
	};

	ResourceManager& resourceManager = Engine::GetEngine().GetResourceManager();

	for (int k = 0; k < 4000; k++)
	{
		char StaticMeshFileName[255];
		sprintf(StaticMeshFileName, "Test.SM_Cube_%d", k);

		ScopedMemoryBlockArray<BYTE> StaticMeshData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(Engine::GetEngine().GetFileSystem().GetFileSize(StaticMeshFileName));
		Engine::GetEngine().GetFileSystem().LoadFile(StaticMeshFileName, StaticMeshData);

		StaticMeshFileHeader *staticMeshFileHeader = (StaticMeshFileHeader*)((BYTE*)StaticMeshData + 2);

		StaticMeshResourceCreateInfo staticMeshResourceCreateInfo;
		staticMeshResourceCreateInfo.VertexCount = staticMeshFileHeader->VertexCount;
		staticMeshResourceCreateInfo.IndexCount = staticMeshFileHeader->IndexCount;
		staticMeshResourceCreateInfo.MeshData = (BYTE*)staticMeshFileHeader + sizeof(StaticMeshFileHeader);

		char StaticMeshResourceName[255];

		sprintf(StaticMeshResourceName, "Cube_%d", k);

		resourceManager.AddResource<StaticMeshResource>(StaticMeshResourceName, &staticMeshResourceCreateInfo);
	}	

	for (int k = 0; k < 4000; k++)
	{
		char Texture2DFileName[255];
		sprintf(Texture2DFileName, "Test.T_Default_%d_D", k);

		ScopedMemoryBlockArray<BYTE> TextureData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(Engine::GetEngine().GetFileSystem().GetFileSize(Texture2DFileName));
		Engine::GetEngine().GetFileSystem().LoadFile(Texture2DFileName, TextureData);

		TextureFileHeader *textureFileHeader = (TextureFileHeader*)((BYTE*)TextureData + 2);

		Texture2DResourceCreateInfo texture2DResourceCreateInfo;
		texture2DResourceCreateInfo.Height = textureFileHeader->Height;
		texture2DResourceCreateInfo.MIPLevels = textureFileHeader->MIPLevels;
		texture2DResourceCreateInfo.SRGB = textureFileHeader->SRGB;
		texture2DResourceCreateInfo.Compressed = textureFileHeader->Compressed;
		texture2DResourceCreateInfo.CompressionType = BlockCompression::BC1;
		texture2DResourceCreateInfo.TexelData = (BYTE*)textureFileHeader + sizeof(TextureFileHeader);
		texture2DResourceCreateInfo.Width = textureFileHeader->Width;

		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);

		resourceManager.AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);
	}	

	for (int k = 0; k < 4000; k++)
	{
		char Texture2DFileName[255];
		sprintf(Texture2DFileName, "Test.T_Default_%d_N", k);

		ScopedMemoryBlockArray<BYTE> TextureData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(Engine::GetEngine().GetFileSystem().GetFileSize(Texture2DFileName));
		Engine::GetEngine().GetFileSystem().LoadFile(Texture2DFileName, TextureData);

		TextureFileHeader *textureFileHeader = (TextureFileHeader*)((BYTE*)TextureData + 2);

		Texture2DResourceCreateInfo texture2DResourceCreateInfo;
		texture2DResourceCreateInfo.Height = textureFileHeader->Height;
		texture2DResourceCreateInfo.MIPLevels = textureFileHeader->MIPLevels;
		texture2DResourceCreateInfo.SRGB = textureFileHeader->SRGB;
		texture2DResourceCreateInfo.Compressed = textureFileHeader->Compressed;
		texture2DResourceCreateInfo.CompressionType = BlockCompression::BC5;
		texture2DResourceCreateInfo.TexelData = (BYTE*)textureFileHeader + sizeof(TextureFileHeader);
		texture2DResourceCreateInfo.Width = textureFileHeader->Width;

		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Normal_%d", k);

		resourceManager.AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);
	}

	for (int k = 0; k < 4000; k++)
	{
		String GraphicsAPI = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueString("System", "GraphicsAPI");

		char MaterialFileName[255];
		sprintf(MaterialFileName, "Test.M_Standart_%d", k);

		ScopedMemoryBlockArray<BYTE> MaterialData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(Engine::GetEngine().GetFileSystem().GetFileSize(MaterialFileName));
		Engine::GetEngine().GetFileSystem().LoadFile(MaterialFileName, MaterialData);

		if (GraphicsAPI == "D3D12")
		{
			char ShaderFileName[255];
			sprintf(ShaderFileName, "ShaderModel51.%s.GBufferOpaquePass_VertexShader", MaterialFileName);

			void *GBufferOpaquePassVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
			SIZE_T GBufferOpaquePassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

			sprintf(ShaderFileName, "ShaderModel51.%s.GBufferOpaquePass_PixelShader", MaterialFileName);

			void *GBufferOpaquePassPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
			SIZE_T GBufferOpaquePassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

			sprintf(ShaderFileName, "ShaderModel51.%s.ShadowMapPass_VertexShader", MaterialFileName);

			void *ShadowMapPassVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
			SIZE_T ShadowMapPassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);
			
			MaterialResourceCreateInfo materialResourceCreateInfo; 
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = GBufferOpaquePassPixelShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = GBufferOpaquePassPixelShaderByteCodeLength;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = GBufferOpaquePassVertexShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = GBufferOpaquePassVertexShaderByteCodeLength;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData = nullptr;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength = 0;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData = ShadowMapPassVertexShaderByteCodeData;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength = ShadowMapPassVertexShaderByteCodeLength;
			materialResourceCreateInfo.Textures.Add(nullptr);
			materialResourceCreateInfo.Textures.Add(nullptr);

			char MaterialResourceName[255];

			sprintf(MaterialResourceName, "Standart_%d", k);

			materialResourceCreateInfo.Textures[0] = resourceManager.GetResource<Texture2DResource>((char*)((BYTE*)MaterialData + 4));
			materialResourceCreateInfo.Textures[1] = resourceManager.GetResource<Texture2DResource>((char*)((BYTE*)MaterialData + 4 + 128));

			resourceManager.AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);
		}
		else if (GraphicsAPI == "Vulkan")
		{
			char ShaderFileName[255];
			sprintf(ShaderFileName, "SPIRV.%s.GBufferOpaquePass_VertexShader", MaterialFileName);

			void *GBufferOpaquePassVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
			SIZE_T GBufferOpaquePassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

			sprintf(ShaderFileName, "SPIRV.%s.GBufferOpaquePass_PixelShader", MaterialFileName);
			
			void *GBufferOpaquePassPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
			SIZE_T GBufferOpaquePassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

			sprintf(ShaderFileName, "SPIRV.%s.ShadowMapPass_VertexShader", MaterialFileName);
			
			void *ShadowMapPassVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
			SIZE_T ShadowMapPassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

			MaterialResourceCreateInfo materialResourceCreateInfo; 
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = GBufferOpaquePassPixelShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = GBufferOpaquePassPixelShaderByteCodeLength;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = GBufferOpaquePassVertexShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = GBufferOpaquePassVertexShaderByteCodeLength;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData = nullptr;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength = 0;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData = ShadowMapPassVertexShaderByteCodeData;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength = ShadowMapPassVertexShaderByteCodeLength;
			materialResourceCreateInfo.Textures.Add(nullptr);
			materialResourceCreateInfo.Textures.Add(nullptr);

			char MaterialResourceName[255];

			sprintf(MaterialResourceName, "Standart_%d", k);

			materialResourceCreateInfo.Textures[0] = resourceManager.GetResource<Texture2DResource>((char*)((BYTE*)MaterialData + 4));
			materialResourceCreateInfo.Textures[1] = resourceManager.GetResource<Texture2DResource>((char*)((BYTE*)MaterialData + 4 + 128));

			resourceManager.AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);
		}				
	}

	HANDLE LevelFile = CreateFile((const wchar_t*)u"GameContent/Maps/000.dmap", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LevelFileSize;
	
	UINT EntitiesCount;
	BOOL Result = ReadFile(LevelFile, &EntitiesCount, sizeof(UINT), NULL, NULL);

	for (UINT i = 0; i < EntitiesCount; i++)
	{
		char EntityClassName[128];

		Result = ReadFile(LevelFile, EntityClassName, 128, NULL, NULL);

		MetaClass *metaClass = Engine::GetEngine().GetGameFramework().GetMetaClassesTable()[EntityClassName];

		//void *entityPtr = Engine::GetEngine().GetMemoryManager().AllocateEntity(metaClass);
		void *entityPtr = SystemAllocator::AllocateMemory(metaClass->GetClassSize());
		metaClass->ObjectConstructorFunc(entityPtr);
		Entity *entity = (Entity*)entityPtr;
		String EntityName = String(metaClass->GetClassName()) + "_" + String((int)metaClass->InstancesCount);
		metaClass->InstancesCount++;
		entity->EntityName = new char[EntityName.GetLength() + 1];
		strcpy((char*)entity->EntityName, EntityName.GetData());
		entity->SetMetaClass(metaClass);
		entity->SetWorld(this);
		entity->LoadFromFile(LevelFile);

		Entities.Add(entity);
	}

	Result = CloseHandle(LevelFile);
}

void World::UnLoadWorld()
{
	Engine::GetEngine().GetResourceManager().DestroyAllResources();
}

Entity* World::SpawnEntity(MetaClass* metaClass)
{
	//void *entityPtr = Engine::GetEngine().GetMemoryManager().AllocateEntity(metaClass);
	void *entityPtr = SystemAllocator::AllocateMemory(metaClass->GetClassSize());
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
	//for (size_t i = 0; i < Entities.GetLength(); i++)
	{
		//Entity* entity = Entities[i];
		if (strcmp(entity->EntityName, EntityName) == 0) return entity;
	}

	return nullptr;
}

void World::TickWorld(float DeltaTime)
{

}