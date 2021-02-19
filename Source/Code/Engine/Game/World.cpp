// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "World.h"

#include "MetaClass.h"
#include "Entity.h"

#include "Entities/Render/Meshes/StaticMeshEntity.h"

#include "Components/Common/TransformComponent.h"
#include "Components/Render/Meshes/StaticMeshComponent.h"

#include <Containers/COMRCPtr.h>

#include <Engine/Engine.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>

void World::LoadWorld()
{
	for (int k = 0; k < 4000; k++)
	{
		char16_t StaticMeshFileName[255];
		wsprintf((wchar_t*)StaticMeshFileName, (const wchar_t*)u"GameContent/Objects/SM_Cube_%d.dasset", k);

		HANDLE StaticMeshFile = CreateFile((const wchar_t*)StaticMeshFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER StaticMeshDataLength;
		BOOL Result = GetFileSizeEx(StaticMeshFile, &StaticMeshDataLength);
		BYTE *StaticMeshData = (BYTE*)malloc(StaticMeshDataLength.QuadPart);
		Result = ReadFile(StaticMeshFile, StaticMeshData, (DWORD)StaticMeshDataLength.QuadPart, NULL, NULL);
		Result = CloseHandle(StaticMeshFile);

		StaticMeshResourceCreateInfo staticMeshResourceCreateInfo;
		staticMeshResourceCreateInfo.VertexCount = *(UINT*)(StaticMeshData);
		staticMeshResourceCreateInfo.IndexCount = *(UINT*)(StaticMeshData + 4);
		staticMeshResourceCreateInfo.VertexData = StaticMeshData + 8;
		staticMeshResourceCreateInfo.IndexData = StaticMeshData + 8 + staticMeshResourceCreateInfo.VertexCount * sizeof(Vertex);

		char StaticMeshResourceName[255];

		sprintf(StaticMeshResourceName, "Cube_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<StaticMeshResource>(StaticMeshResourceName, &staticMeshResourceCreateInfo);

		free(StaticMeshData);
	}	

	for (int k = 0; k < 4000; k++)
	{
		char16_t Texture2DFileName[255];
		wsprintf((wchar_t*)Texture2DFileName, (const wchar_t*)u"GameContent/Textures/T_Default_%d_D.dasset", k);

		HANDLE TextureFile = CreateFile((const wchar_t*)Texture2DFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER TextureDataLength;
		BOOL Result = GetFileSizeEx(TextureFile, &TextureDataLength);
		BYTE *TextureData = (BYTE*)malloc(TextureDataLength.QuadPart);
		Result = ReadFile(TextureFile, TextureData, (DWORD)TextureDataLength.QuadPart, NULL, NULL);
		Result = CloseHandle(TextureFile);

		Texture2DResourceCreateInfo texture2DResourceCreateInfo;
		texture2DResourceCreateInfo.Height = *(UINT*)(TextureData + 4);
		texture2DResourceCreateInfo.MIPLevels = *(UINT*)(TextureData + 8);
		texture2DResourceCreateInfo.SRGB = *(BOOL*)(TextureData + 12);
		texture2DResourceCreateInfo.Compressed = *(BOOL*)(TextureData + 16);
		texture2DResourceCreateInfo.CompressionType = BlockCompression::BC1;
		texture2DResourceCreateInfo.TexelData = TextureData + 20;
		texture2DResourceCreateInfo.Width = *(UINT*)(TextureData);

		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);

		free(TextureData);
	}	

	for (int k = 0; k < 4000; k++)
	{
		char16_t Texture2DFileName[255];
		wsprintf((wchar_t*)Texture2DFileName, (const wchar_t*)u"GameContent/Textures/T_Default_%d_N.dasset", k);

		HANDLE TextureFile = CreateFile((const wchar_t*)Texture2DFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER TextureDataLength;
		BOOL Result = GetFileSizeEx(TextureFile, &TextureDataLength);
		BYTE *TextureData = (BYTE*)malloc(TextureDataLength.QuadPart);
		Result = ReadFile(TextureFile, TextureData, (DWORD)TextureDataLength.QuadPart, NULL, NULL);
		Result = CloseHandle(TextureFile);

		Texture2DResourceCreateInfo texture2DResourceCreateInfo;
		texture2DResourceCreateInfo.Height = *(UINT*)(TextureData + 4);
		texture2DResourceCreateInfo.MIPLevels = *(UINT*)(TextureData + 8);
		texture2DResourceCreateInfo.SRGB = *(BOOL*)(TextureData + 12);
		texture2DResourceCreateInfo.Compressed = *(BOOL*)(TextureData + 16);
		texture2DResourceCreateInfo.CompressionType = BlockCompression::BC5;
		texture2DResourceCreateInfo.TexelData = TextureData + 20;
		texture2DResourceCreateInfo.Width = *(UINT*)(TextureData);

		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Normal_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);

		free(TextureData);
	}	

	for (int k = 0; k < 4000; k++)
	{
		HANDLE VertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_VertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER VertexShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(VertexShaderFile, &VertexShaderByteCodeLength);
		void *VertexShaderByteCodeData = malloc(VertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(VertexShaderFile, VertexShaderByteCodeData, (DWORD)VertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(VertexShaderFile);

		HANDLE PixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_PixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER PixelShaderByteCodeLength;
		Result = GetFileSizeEx(PixelShaderFile, &PixelShaderByteCodeLength);
		void *PixelShaderByteCodeData = malloc(PixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(PixelShaderFile, PixelShaderByteCodeData, (DWORD)PixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(PixelShaderFile);

		MaterialResourceCreateInfo materialResourceCreateInfo;
		materialResourceCreateInfo.PixelShaderByteCodeData = PixelShaderByteCodeData;
		materialResourceCreateInfo.PixelShaderByteCodeLength = PixelShaderByteCodeLength.QuadPart;
		materialResourceCreateInfo.VertexShaderByteCodeData = VertexShaderByteCodeData;
		materialResourceCreateInfo.VertexShaderByteCodeLength = VertexShaderByteCodeLength.QuadPart;
		materialResourceCreateInfo.Textures.resize(2);

		char MaterialResourceName[255];

		sprintf(MaterialResourceName, "Standart_%d", k);

		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);
		materialResourceCreateInfo.Textures[0] = Engine::GetEngine().GetResourceManager().GetResource<Texture2DResource>(Texture2DResourceName);
		sprintf(Texture2DResourceName, "Normal_%d", k);
		materialResourceCreateInfo.Textures[1] = Engine::GetEngine().GetResourceManager().GetResource<Texture2DResource>(Texture2DResourceName);

		Engine::GetEngine().GetResourceManager().AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);

		free(VertexShaderByteCodeData);
		free(PixelShaderByteCodeData);
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

			StaticMeshEntity *staticMeshEntity = Entity::DynamicCast<StaticMeshEntity>(SpawnEntity(StaticMeshEntity::GetMetaClassStatic()));
			staticMeshEntity->GetTransformComponent()->SetLocation(XMFLOAT3(i * 5.0f + 2.5f, -0.0f, j * 5.0f + 2.5f));
			staticMeshEntity->GetStaticMeshComponent()->SetStaticMesh(Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshEntity->GetStaticMeshComponent()->SetMaterial(Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;

			sprintf(StaticMeshResourceName, "Cube_%d", ResourceCounter);
			sprintf(MaterialResourceName, "Standart_%d", ResourceCounter);

			staticMeshEntity = Entity::DynamicCast<StaticMeshEntity>(SpawnEntity(StaticMeshEntity::GetMetaClassStatic()));
			staticMeshEntity->GetTransformComponent()->SetLocation(XMFLOAT3(i * 10.0f + 5.0f, -2.0f, j * 10.0f + 5.0f));
			staticMeshEntity->GetTransformComponent()->SetScale(XMFLOAT3(5.0f, 1.0f, 5.0f));
			staticMeshEntity->GetStaticMeshComponent()->SetStaticMesh(Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshEntity->GetStaticMeshComponent()->SetMaterial(Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;
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