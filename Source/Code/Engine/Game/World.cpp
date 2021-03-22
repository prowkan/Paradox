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
		char16_t StaticMeshFileName[255];
		wsprintf((wchar_t*)StaticMeshFileName, (const wchar_t*)u"GameContent/Objects/SM_Cube_%d.dasset", k);

		HANDLE StaticMeshFile = CreateFile((const wchar_t*)StaticMeshFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER StaticMeshDataLength;
		BOOL Result = GetFileSizeEx(StaticMeshFile, &StaticMeshDataLength);
		ScopedMemoryBlockArray<BYTE> StaticMeshData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(StaticMeshDataLength.QuadPart);
		Result = ReadFile(StaticMeshFile, StaticMeshData, (DWORD)StaticMeshDataLength.QuadPart, NULL, NULL);
		Result = CloseHandle(StaticMeshFile);

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
		char16_t Texture2DFileName[255];
		wsprintf((wchar_t*)Texture2DFileName, (const wchar_t*)u"GameContent/Textures/T_Default_%d_D.dasset", k);

		HANDLE TextureFile = CreateFile((const wchar_t*)Texture2DFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER TextureDataLength;
		BOOL Result = GetFileSizeEx(TextureFile, &TextureDataLength);
		ScopedMemoryBlockArray<BYTE> TextureData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(TextureDataLength.QuadPart);
		Result = ReadFile(TextureFile, TextureData, (DWORD)TextureDataLength.QuadPart, NULL, NULL);
		Result = CloseHandle(TextureFile);

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
		char16_t Texture2DFileName[255];
		wsprintf((wchar_t*)Texture2DFileName, (const wchar_t*)u"GameContent/Textures/T_Default_%d_N.dasset", k);

		HANDLE TextureFile = CreateFile((const wchar_t*)Texture2DFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER TextureDataLength;
		BOOL Result = GetFileSizeEx(TextureFile, &TextureDataLength);
		ScopedMemoryBlockArray<BYTE> TextureData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(TextureDataLength.QuadPart);
		Result = ReadFile(TextureFile, TextureData, (DWORD)TextureDataLength.QuadPart, NULL, NULL);
		Result = CloseHandle(TextureFile);

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
		HANDLE GBufferOpaquePassVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_VertexShader_GBufferOpaquePass.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER GBufferOpaquePassVertexShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(GBufferOpaquePassVertexShaderFile, &GBufferOpaquePassVertexShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> GBufferOpaquePassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassVertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(GBufferOpaquePassVertexShaderFile, GBufferOpaquePassVertexShaderByteCodeData, (DWORD)GBufferOpaquePassVertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(GBufferOpaquePassVertexShaderFile);

		HANDLE GBufferOpaquePassPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_PixelShader_GBufferOpaquePass.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER GBufferOpaquePassPixelShaderByteCodeLength;
		Result = GetFileSizeEx(GBufferOpaquePassPixelShaderFile, &GBufferOpaquePassPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> GBufferOpaquePassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(GBufferOpaquePassPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(GBufferOpaquePassPixelShaderFile, GBufferOpaquePassPixelShaderByteCodeData, (DWORD)GBufferOpaquePassPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(GBufferOpaquePassPixelShaderFile);

		HANDLE ShadowMapPassVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_VertexShader_ShadowMapPass.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ShadowMapPassVertexShaderByteCodeLength;
		Result = GetFileSizeEx(ShadowMapPassVertexShaderFile, &ShadowMapPassVertexShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> ShadowMapPassVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowMapPassVertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(ShadowMapPassVertexShaderFile, ShadowMapPassVertexShaderByteCodeData, (DWORD)ShadowMapPassVertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(ShadowMapPassVertexShaderFile);

		MaterialResourceCreateInfo materialResourceCreateInfo;
		materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = GBufferOpaquePassPixelShaderByteCodeData;
		materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = GBufferOpaquePassPixelShaderByteCodeLength.QuadPart;
		materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = GBufferOpaquePassVertexShaderByteCodeData;
		materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = GBufferOpaquePassVertexShaderByteCodeLength.QuadPart;
		materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData = nullptr;
		materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength = 0;
		materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData = ShadowMapPassVertexShaderByteCodeData;
		materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength = ShadowMapPassVertexShaderByteCodeLength.QuadPart;
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