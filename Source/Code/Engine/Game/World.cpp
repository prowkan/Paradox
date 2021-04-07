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
		sprintf(StaticMeshFileName, "SM_Cube_%d", k);

		ScopedMemoryBlockArray<BYTE> StaticMeshData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(Engine::GetEngine().GetFileSystem().GetFileSize(StaticMeshFileName));
		Engine::GetEngine().GetFileSystem().LoadFile(StaticMeshFileName, StaticMeshData);

		StaticMeshFileHeader *staticMeshFileHeader = StaticMeshData;

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
		sprintf(Texture2DFileName, "T_Default_%d_D", k);

		ScopedMemoryBlockArray<BYTE> TextureData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(Engine::GetEngine().GetFileSystem().GetFileSize(Texture2DFileName));
		Engine::GetEngine().GetFileSystem().LoadFile(Texture2DFileName, TextureData);

		TextureFileHeader *textureFileHeader = TextureData;

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
		sprintf(Texture2DFileName, "T_Default_%d_N", k);

		ScopedMemoryBlockArray<BYTE> TextureData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(Engine::GetEngine().GetFileSystem().GetFileSize(Texture2DFileName));
		Engine::GetEngine().GetFileSystem().LoadFile(Texture2DFileName, TextureData);

		TextureFileHeader *textureFileHeader = TextureData;

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
		string GraphicsAPI = Engine::GetEngine().GetConfigSystem().GetRenderConfigValueString("System", "GraphicsAPI");

		if (GraphicsAPI == "D3D12")
		{
			SIZE_T GBufferOpaquePassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("ShaderModel51.MaterialBase_VertexShader_GBufferOpaquePass");
			ScopedMemoryBlockArray<BYTE> GBufferOpaquePassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassVertexShaderByteCodeLength);
			Engine::GetEngine().GetFileSystem().LoadFile("ShaderModel51.MaterialBase_VertexShader_GBufferOpaquePass", GBufferOpaquePassVertexShaderByteCodeData);

			SIZE_T GBufferOpaquePassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("ShaderModel51.MaterialBase_PixelShader_GBufferOpaquePass");
			ScopedMemoryBlockArray<BYTE> GBufferOpaquePassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassPixelShaderByteCodeLength);
			Engine::GetEngine().GetFileSystem().LoadFile("ShaderModel51.MaterialBase_PixelShader_GBufferOpaquePass", GBufferOpaquePassPixelShaderByteCodeData);

			SIZE_T ShadowMapPassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("ShaderModel51.MaterialBase_VertexShader_ShadowMapPass");
			ScopedMemoryBlockArray<BYTE> ShadowMapPassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowMapPassVertexShaderByteCodeLength);
			Engine::GetEngine().GetFileSystem().LoadFile("ShaderModel51.MaterialBase_VertexShader_ShadowMapPass", ShadowMapPassVertexShaderByteCodeData);

			MaterialResourceCreateInfo materialResourceCreateInfo; 
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = GBufferOpaquePassPixelShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = GBufferOpaquePassPixelShaderByteCodeLength;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = GBufferOpaquePassVertexShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = GBufferOpaquePassVertexShaderByteCodeLength;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData = nullptr;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength = 0;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData = ShadowMapPassVertexShaderByteCodeData;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength = ShadowMapPassVertexShaderByteCodeLength;
			materialResourceCreateInfo.Textures.resize(2);

			char MaterialResourceName[255];

			sprintf(MaterialResourceName, "Standart_%d", k);

			char Texture2DResourceName[255];

			sprintf(Texture2DResourceName, "Checker_%d", k);
			materialResourceCreateInfo.Textures[0] = resourceManager.GetResource<Texture2DResource>(Texture2DResourceName);
			sprintf(Texture2DResourceName, "Normal_%d", k);
			materialResourceCreateInfo.Textures[1] = resourceManager.GetResource<Texture2DResource>(Texture2DResourceName);

			resourceManager.AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);
		}
		else if (GraphicsAPI == "Vulkan")
		{
			SIZE_T GBufferOpaquePassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.MaterialBase_VertexShader_GBufferOpaquePass");
			ScopedMemoryBlockArray<BYTE> GBufferOpaquePassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassVertexShaderByteCodeLength);
			Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.MaterialBase_VertexShader_GBufferOpaquePass", GBufferOpaquePassVertexShaderByteCodeData);

			SIZE_T GBufferOpaquePassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.MaterialBase_PixelShader_GBufferOpaquePass");
			ScopedMemoryBlockArray<BYTE> GBufferOpaquePassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassPixelShaderByteCodeLength);
			Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.MaterialBase_PixelShader_GBufferOpaquePass", GBufferOpaquePassPixelShaderByteCodeData);

			SIZE_T ShadowMapPassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.MaterialBase_VertexShader_ShadowMapPass");
			ScopedMemoryBlockArray<BYTE> ShadowMapPassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowMapPassVertexShaderByteCodeLength);
			Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.MaterialBase_VertexShader_ShadowMapPass", ShadowMapPassVertexShaderByteCodeData);

			MaterialResourceCreateInfo materialResourceCreateInfo; 
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = GBufferOpaquePassPixelShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = GBufferOpaquePassPixelShaderByteCodeLength;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = GBufferOpaquePassVertexShaderByteCodeData;
			materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = GBufferOpaquePassVertexShaderByteCodeLength;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData = nullptr;
			materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength = 0;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData = ShadowMapPassVertexShaderByteCodeData;
			materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength = ShadowMapPassVertexShaderByteCodeLength;
			materialResourceCreateInfo.Textures.resize(2);

			char MaterialResourceName[255];

			sprintf(MaterialResourceName, "Standart_%d", k);

			char Texture2DResourceName[255];

			sprintf(Texture2DResourceName, "Checker_%d", k);
			materialResourceCreateInfo.Textures[0] = resourceManager.GetResource<Texture2DResource>(Texture2DResourceName);
			sprintf(Texture2DResourceName, "Normal_%d", k);
			materialResourceCreateInfo.Textures[1] = resourceManager.GetResource<Texture2DResource>(Texture2DResourceName);

			resourceManager.AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);
		}				
	}

	HANDLE LevelFile = CreateFile((const wchar_t*)u"WorldChunks/000.worldchunk", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LevelFileSize;
	
	UINT EntitiesCount;
	BOOL Result = ReadFile(LevelFile, &EntitiesCount, sizeof(UINT), NULL, NULL);

	for (UINT i = 0; i < EntitiesCount; i++)
	{
		char EntityClassName[128];

		Result = ReadFile(LevelFile, EntityClassName, 128, NULL, NULL);

		MetaClass *metaClass = Engine::GetEngine().GetGameFramework().GetMetaClassesTable()[EntityClassName];

		//void *entityPtr = Engine::GetEngine().GetMemoryManager().AllocateEntity(metaClass);
		void *entityPtr = malloc(metaClass->GetClassSize());
		metaClass->ObjectConstructorFunc(entityPtr);
		Entity *entity = (Entity*)entityPtr;
		string EntityName = string(metaClass->GetClassName()) + "_" + to_string(metaClass->InstancesCount);
		metaClass->InstancesCount++;
		entity->EntityName = new char[EntityName.length() + 1];
		strcpy((char*)entity->EntityName, EntityName.c_str());
		entity->SetMetaClass(metaClass);
		entity->SetWorld(this);
		entity->LoadFromFile(LevelFile);

		Entities.push_back(entity);
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
	void *entityPtr = malloc(metaClass->GetClassSize());
	metaClass->ObjectConstructorFunc(entityPtr);
	Entity *entity = (Entity*)entityPtr;
	string EntityName = string(metaClass->GetClassName()) + "_" + to_string(metaClass->InstancesCount);
	metaClass->InstancesCount++;
	entity->EntityName = new char[EntityName.length() + 1];
	strcpy((char*)entity->EntityName, EntityName.c_str());
	entity->SetMetaClass(metaClass);
	entity->SetWorld(this);
	entity->InitDefaultProperties();
	Entities.push_back(entity);
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