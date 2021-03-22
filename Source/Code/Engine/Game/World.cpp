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
		staticMeshResourceCreateInfo.VertexData = (BYTE*)staticMeshFileHeader + sizeof(StaticMeshFileHeader);
		staticMeshResourceCreateInfo.IndexData = (BYTE*)staticMeshFileHeader + sizeof(StaticMeshFileHeader) + staticMeshResourceCreateInfo.VertexCount * sizeof(Vertex);

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
		SIZE_T GBufferOpaquePassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("MaterialBase_VertexShader_GBufferOpaquePass");
		ScopedMemoryBlockArray<BYTE> GBufferOpaquePassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassVertexShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("MaterialBase_VertexShader_GBufferOpaquePass", GBufferOpaquePassVertexShaderByteCodeData);

		SIZE_T GBufferOpaquePassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("MaterialBase_PixelShader_GBufferOpaquePass");
		ScopedMemoryBlockArray<BYTE> GBufferOpaquePassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("MaterialBase_PixelShader_GBufferOpaquePass", GBufferOpaquePassPixelShaderByteCodeData);

		SIZE_T ShadowMapPassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("MaterialBase_VertexShader_ShadowMapPass");
		ScopedMemoryBlockArray<BYTE> ShadowMapPassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowMapPassVertexShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("MaterialBase_VertexShader_ShadowMapPass", ShadowMapPassVertexShaderByteCodeData);

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

	UINT ResourceCounter = 0;

	for (int i = -50; i < 50; i++)
	{
		for (int j = -50; j < 50; j++)
		{
			char StaticMeshResourceName[255];
			char MaterialResourceName[255];

			sprintf(StaticMeshResourceName, "Cube_%u", ResourceCounter);
			sprintf(MaterialResourceName, "Standart_%u", ResourceCounter);

			StaticMeshEntity *staticMeshEntity = SpawnEntity<StaticMeshEntity>();
			staticMeshEntity->GetTransformComponent()->SetLocation(XMFLOAT3(i * 5.0f + 2.5f, -0.0f, j * 5.0f + 2.5f));
			staticMeshEntity->GetStaticMeshComponent()->SetStaticMesh(resourceManager.GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshEntity->GetStaticMeshComponent()->SetMaterial(resourceManager.GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;

			sprintf(StaticMeshResourceName, "Cube_%d", ResourceCounter);
			sprintf(MaterialResourceName, "Standart_%d", ResourceCounter);

			staticMeshEntity = SpawnEntity<StaticMeshEntity>();
			staticMeshEntity->GetTransformComponent()->SetLocation(XMFLOAT3(i * 10.0f + 5.0f, -2.0f, j * 10.0f + 5.0f));
			staticMeshEntity->GetTransformComponent()->SetScale(XMFLOAT3(5.0f, 1.0f, 5.0f));
			staticMeshEntity->GetStaticMeshComponent()->SetStaticMesh(resourceManager.GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshEntity->GetStaticMeshComponent()->SetMaterial(resourceManager.GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;

			PointLightEntity *pointLightEntity = SpawnEntity<PointLightEntity>();
			pointLightEntity->GetTransformComponent()->SetLocation(XMFLOAT3(i * 10.0f + 5.0f, 1.5f, j * 10.0f + 5.0f));
			pointLightEntity->GetPointLightComponent()->SetBrightness(10.0f);
			pointLightEntity->GetPointLightComponent()->SetRadius(5.0f);
			pointLightEntity->GetPointLightComponent()->SetColor(XMFLOAT3((i + 51) / 100.0f, 0.1f, (j + 51) / 100.0f));
		}
	}
}

void World::UnLoadWorld()
{
	Engine::GetEngine().GetResourceManager().DestroyAllResources();
}

Entity* World::SpawnEntity(MetaClass* metaClass)
{
	void *entityPtr = Engine::GetEngine().GetMemoryManager().AllocateEntity(metaClass);
	metaClass->ObjectConstructorFunc(entityPtr);
	Entity *entity = (Entity*)entityPtr;
	entity->SetMetaClass(metaClass);
	entity->SetWorld(this);
	entity->InitDefaultProperties();
	Entities.push_back(entity);
	return entity;
}


void World::TickWorld(float DeltaTime)
{

}