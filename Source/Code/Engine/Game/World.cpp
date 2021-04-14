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
	const int VertexCount = 9 * 9 * 6;
	const int IndexCount = 8 * 8 * 6 * 6;

	Vertex Vertices[VertexCount];

	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			Vertices[0 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, 1.0f - i * 0.25f, -1.0f);
			Vertices[0 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[81 + 9 * i + j].Position = XMFLOAT3(1.0f, 1.0f - i * 0.25f, -1.0f + j * 0.25f);
			Vertices[81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[2 * 81 + 9 * i + j].Position = XMFLOAT3(1.0f - j * 0.25f, 1.0f - i * 0.25f, 1.0f);
			Vertices[2 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[3 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f, 1.0f - i * 0.25f, 1.0f - j * 0.25f);
			Vertices[3 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[4 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, 1.0f, 1.0f - i * 0.25f);
			Vertices[4 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);

			Vertices[5 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, -1.0f, -1.0f + i * 0.25f);
			Vertices[5 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
		}
	}

	WORD Indices[IndexCount];

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 0 * 81 + 9 * i + j;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 0 * 81 + 9 * i + j + 1;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 0 * 81 + 9 * (i + 1) + j;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 0 * 81 + 9 * (i + 1) + j;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 0 * 81 + 9 * i + j + 1;
			Indices[0 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 0 * 81 + 9 * (i + 1) + j + 1;

			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 1 * 81 + 9 * i + j;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 1 * 81 + 9 * i + j + 1;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 1 * 81 + 9 * (i + 1) + j;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 1 * 81 + 9 * (i + 1) + j;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 1 * 81 + 9 * i + j + 1;
			Indices[1 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 1 * 81 + 9 * (i + 1) + j + 1;

			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 2 * 81 + 9 * i + j;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 2 * 81 + 9 * i + j + 1;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 2 * 81 + 9 * (i + 1) + j;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 2 * 81 + 9 * (i + 1) + j;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 2 * 81 + 9 * i + j + 1;
			Indices[2 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 2 * 81 + 9 * (i + 1) + j + 1;

			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 3 * 81 + 9 * i + j;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 3 * 81 + 9 * i + j + 1;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 3 * 81 + 9 * (i + 1) + j;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 3 * 81 + 9 * (i + 1) + j;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 3 * 81 + 9 * i + j + 1;
			Indices[3 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 3 * 81 + 9 * (i + 1) + j + 1;

			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 4 * 81 + 9 * i + j;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 4 * 81 + 9 * i + j + 1;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 4 * 81 + 9 * (i + 1) + j;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 4 * 81 + 9 * (i + 1) + j;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 4 * 81 + 9 * i + j + 1;
			Indices[4 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 4 * 81 + 9 * (i + 1) + j + 1;

			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 0] = 5 * 81 + 9 * i + j;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 1] = 5 * 81 + 9 * i + j + 1;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 2] = 5 * 81 + 9 * (i + 1) + j;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 3] = 5 * 81 + 9 * (i + 1) + j;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 4] = 5 * 81 + 9 * i + j + 1;
			Indices[5 * 8 * 8 * 6 + 8 * 6 * i + 6 * j + 5] = 5 * 81 + 9 * (i + 1) + j + 1;
		}
	}

	StaticMeshResourceCreateInfo staticMeshResourceCreateInfo;
	staticMeshResourceCreateInfo.IndexCount = IndexCount;
	staticMeshResourceCreateInfo.IndexData = Indices;
	staticMeshResourceCreateInfo.VertexCount = VertexCount;
	staticMeshResourceCreateInfo.VertexData = Vertices;

	for (int k = 0; k < 4000; k++)
	{
		char StaticMeshResourceName[255];

		sprintf(StaticMeshResourceName, "Cube_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<StaticMeshResource>(StaticMeshResourceName, &staticMeshResourceCreateInfo);
	}

	ScopedMemoryBlockArray<Texel> TexelData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(512 * 512 + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4);

	Texel *Texels[8];

	Texels[0] = TexelData;
	Texels[1] = Texels[0] + 512 * 512;
	Texels[2] = Texels[1] + 256 * 256;
	Texels[3] = Texels[2] + 128 * 128;
	Texels[4] = Texels[3] + 64 * 64;
	Texels[5] = Texels[4] + 32 * 32;
	Texels[6] = Texels[5] + 16 * 16;
	Texels[7] = Texels[6] + 8 * 8;

	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			Texels[0][y * 512 + x].R = ((x / 64) + (y / 64)) % 2 ? 0 : 255;
			Texels[0][y * 512 + x].G = ((x / 64) + (y / 64)) % 2 ? 0 : 255;
			Texels[0][y * 512 + x].B = ((x / 64) + (y / 64)) % 2 ? 192 : 255;
			Texels[0][y * 512 + x].A = 255;
		}
	}

	for (int k = 1; k < 8; k++)
	{
		int MIPSize = 512 >> k;

		for (int y = 0; y < MIPSize; y++)
		{
			for (int x = 0; x < MIPSize; x++)
			{
				Texels[k][y * MIPSize + x].R = BYTE(0.25f * ((float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x)].R + (float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x + 1)].R + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x)].R + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x + 1)].R));
				Texels[k][y * MIPSize + x].G = BYTE(0.25f * ((float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x)].G + (float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x + 1)].G + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x)].G + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x + 1)].G));
				Texels[k][y * MIPSize + x].B = BYTE(0.25f * ((float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x)].B + (float)Texels[k - 1][(2 * y) * (2 * MIPSize) + (2 * x + 1)].B + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x)].B + (float)Texels[k - 1][(2 * y + 1) * (2 * MIPSize) + (2 * x + 1)].B));
				Texels[k][y * MIPSize + x].A = 255;
			}
		}
	}

	ScopedMemoryBlockArray<CompressedTexelBlock> CompressedTexelBlockData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<CompressedTexelBlock>(128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1 * 1);

	CompressedTexelBlock *CompressedTexelBlocks[8];

	CompressedTexelBlocks[0] = CompressedTexelBlockData;
	CompressedTexelBlocks[1] = CompressedTexelBlocks[0] + 128 * 128;
	CompressedTexelBlocks[2] = CompressedTexelBlocks[1] + 64 * 64;
	CompressedTexelBlocks[3] = CompressedTexelBlocks[2] + 32 * 32;
	CompressedTexelBlocks[4] = CompressedTexelBlocks[3] + 16 * 16;
	CompressedTexelBlocks[5] = CompressedTexelBlocks[4] + 8 * 8;
	CompressedTexelBlocks[6] = CompressedTexelBlocks[5] + 4 * 4;
	CompressedTexelBlocks[7] = CompressedTexelBlocks[6] + 2 * 2;

	for (int k = 0; k < 8; k++)
	{
		int MIPSize = 128 >> k;

		for (int y = 0; y < MIPSize; y++)
		{
			for (int x = 0; x < MIPSize; x++)
			{
				Color MinColor{ 255, 255, 255 }, MaxColor{ 0, 0, 0 };

				for (int j = 0; j < 4; j++)
				{
					for (int i = 0; i < 4; i++)
					{
						if (Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R < MinColor.R) MinColor.R = Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R;
						if (Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G < MinColor.G) MinColor.G = Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G;
						if (Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].B < MinColor.B) MinColor.B = Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].B;

						if (Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R > MaxColor.R) MaxColor.R = Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R;
						if (Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G > MaxColor.G) MaxColor.G = Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G;
						if (Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].B > MaxColor.B) MaxColor.B = Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].B;
					}
				}

				Color Colors[4];
				Colors[0] = MinColor;
				Colors[1] = MaxColor;
				Colors[2] = 2 * Colors[0] / 3 + Colors[1] / 3;
				Colors[3] = Colors[0] / 3 + 2 * Colors[1] / 3;

				CompressedTexelBlocks[k][y * MIPSize + x].Colors[0] = 0;
				CompressedTexelBlocks[k][y * MIPSize + x].Colors[1] = 0;

				CompressedTexelBlocks[k][y * MIPSize + x].Colors[0] |= ((((BYTE)(((float)MinColor.R / 255.0f) * 31.0f)) & 0b11111) << 11);
				CompressedTexelBlocks[k][y * MIPSize + x].Colors[0] |= ((((BYTE)(((float)MinColor.G / 255.0f) * 63.0f)) & 0b111111) << 5);
				CompressedTexelBlocks[k][y * MIPSize + x].Colors[0] |= ((((BYTE)(((float)MinColor.B / 255.0f) * 31.0f)) & 0b11111));

				CompressedTexelBlocks[k][y * MIPSize + x].Colors[1] |= ((((BYTE)(((float)MaxColor.R / 255.0f) * 31.0f)) & 0b11111) << 11);
				CompressedTexelBlocks[k][y * MIPSize + x].Colors[1] |= ((((BYTE)(((float)MaxColor.G / 255.0f) * 63.0f)) & 0b111111) << 5);
				CompressedTexelBlocks[k][y * MIPSize + x].Colors[1] |= ((((BYTE)(((float)MaxColor.B / 255.0f) * 31.0f)) & 0b11111));

				CompressedTexelBlocks[k][y * MIPSize + x].Texels[0] = 0;
				CompressedTexelBlocks[k][y * MIPSize + x].Texels[1] = 0;
				CompressedTexelBlocks[k][y * MIPSize + x].Texels[2] = 0;
				CompressedTexelBlocks[k][y * MIPSize + x].Texels[3] = 0;

				for (int j = 0; j < 4; j++)
				{
					for (int i = 0; i < 4; i++)
					{
						Color TexelColor{ (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R, (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G, (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].B };

						float Dist = DistanceBetweenColor(TexelColor, Colors[0]);
						uint8_t ArgMin = 0;

						for (uint8_t x = 1; x < 4; x++)
						{
							float NewDist = DistanceBetweenColor(TexelColor, Colors[x]);

							if (NewDist < Dist)
							{
								Dist = NewDist;
								ArgMin = x;
							}
						}

						CompressedTexelBlocks[k][y * MIPSize + x].Texels[j] |= ((ArgMin & 0b11) << (2 * i));
					}
				}
			}
		}
	}

	Texture2DResourceCreateInfo texture2DResourceCreateInfo;
	texture2DResourceCreateInfo.Height = 512;
	texture2DResourceCreateInfo.MIPLevels = 8;
	texture2DResourceCreateInfo.SRGB = TRUE;
	texture2DResourceCreateInfo.Compressed = TRUE;
	texture2DResourceCreateInfo.TexelData = (BYTE*)CompressedTexelBlockData;
	texture2DResourceCreateInfo.Width = 512;

	for (int k = 0; k < 4000; k++)
	{
		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);
	}

	HANDLE VertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_VertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER VertexShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(VertexShaderFile, &VertexShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> VertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(VertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(VertexShaderFile, VertexShaderByteCodeData, (DWORD)VertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(VertexShaderFile);

	HANDLE PixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/MaterialBase_PixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER PixelShaderByteCodeLength;
	Result = GetFileSizeEx(PixelShaderFile, &PixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> PixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(PixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(PixelShaderFile, PixelShaderByteCodeData, (DWORD)PixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(PixelShaderFile);

	MaterialResourceCreateInfo materialResourceCreateInfo;
	materialResourceCreateInfo.PixelShaderByteCodeData = PixelShaderByteCodeData;
	materialResourceCreateInfo.PixelShaderByteCodeLength = PixelShaderByteCodeLength.QuadPart;
	materialResourceCreateInfo.VertexShaderByteCodeData = VertexShaderByteCodeData;
	materialResourceCreateInfo.VertexShaderByteCodeLength = VertexShaderByteCodeLength.QuadPart;
	materialResourceCreateInfo.Textures.resize(1);

	for (int k = 0; k < 4000; k++)
	{
		char MaterialResourceName[255];

		sprintf(MaterialResourceName, "Standart_%d", k);

		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);

		materialResourceCreateInfo.Textures[0] = Engine::GetEngine().GetResourceManager().GetResource<Texture2DResource>(Texture2DResourceName);

		Engine::GetEngine().GetResourceManager().AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);
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
			staticMeshEntity->GetStaticMeshComponent()->SetStaticMesh(Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshEntity->GetStaticMeshComponent()->SetMaterial(Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;

			sprintf(StaticMeshResourceName, "Cube_%d", ResourceCounter);
			sprintf(MaterialResourceName, "Standart_%d", ResourceCounter);

			staticMeshEntity = SpawnEntity<StaticMeshEntity>();
			staticMeshEntity->GetTransformComponent()->SetLocation(XMFLOAT3(i * 10.0f + 5.0f, -2.0f, j * 10.0f + 5.0f));
			staticMeshEntity->GetTransformComponent()->SetScale(XMFLOAT3(5.0f, 1.0f, 5.0f));
			staticMeshEntity->GetStaticMeshComponent()->SetStaticMesh(Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshEntity->GetStaticMeshComponent()->SetMaterial(Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;

			PointLightEntity *pointLightEntity = SpawnEntity<PointLightEntity>();
			pointLightEntity->GetTransformComponent()->SetLocation(XMFLOAT3(i * 10.0f + 5.0f, 1.5f, j * 10.0f + 5.0f));
			pointLightEntity->GetPointLightComponent()->SetBrightness(10.0f);
			pointLightEntity->GetPointLightComponent()->SetRadius(5.0f);
			pointLightEntity->GetPointLightComponent()->SetColor(XMFLOAT3((i + 51) / 100.0f, 0.1f, (j + 51) / 100.0f));
		}
	}

	Engine::GetEngine().GetRenderSystem().PrepareData();
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