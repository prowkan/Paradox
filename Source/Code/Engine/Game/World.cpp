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
	const int VertexCount = 9 * 9 * 6;
	const int IndexCount = 8 * 8 * 6 * 6;

	Vertex Vertices[VertexCount];

	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			Vertices[0 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, 1.0f - i * 0.25f, -1.0f);
			Vertices[0 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
			Vertices[0 + 9 * i + j].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
			Vertices[0 + 9 * i + j].Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
			Vertices[0 + 9 * i + j].Binormal = XMFLOAT3(0.0f, -1.0f, 0.0f);

			Vertices[81 + 9 * i + j].Position = XMFLOAT3(1.0f, 1.0f - i * 0.25f, -1.0f + j * 0.25f);
			Vertices[81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
			Vertices[81 + 9 * i + j].Normal = XMFLOAT3(1.0f, 0.0f, 0.0f);
			Vertices[81 + 9 * i + j].Tangent = XMFLOAT3(0.0f, 0.0f, 1.0f);
			Vertices[81 + 9 * i + j].Binormal = XMFLOAT3(0.0f, -1.0f, 0.0f);

			Vertices[2 * 81 + 9 * i + j].Position = XMFLOAT3(1.0f - j * 0.25f, 1.0f - i * 0.25f, 1.0f);
			Vertices[2 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
			Vertices[2 * 81 + 9 * i + j].Normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
			Vertices[2 * 81 + 9 * i + j].Tangent = XMFLOAT3(-1.0f, 0.0f, 0.0f);
			Vertices[2 * 81 + 9 * i + j].Binormal = XMFLOAT3(0.0f, -1.0f, 0.0f);

			Vertices[3 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f, 1.0f - i * 0.25f, 1.0f - j * 0.25f);
			Vertices[3 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
			Vertices[3 * 81 + 9 * i + j].Normal = XMFLOAT3(-1.0f, 0.0f, 0.0f);
			Vertices[3 * 81 + 9 * i + j].Tangent = XMFLOAT3(0.0f, 0.0f, -1.0f);
			Vertices[3 * 81 + 9 * i + j].Binormal = XMFLOAT3(0.0f, -1.0f, 0.0f);

			Vertices[4 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, 1.0f, 1.0f - i * 0.25f);
			Vertices[4 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
			Vertices[4 * 81 + 9 * i + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			Vertices[4 * 81 + 9 * i + j].Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
			Vertices[4 * 81 + 9 * i + j].Binormal = XMFLOAT3(0.0f, 0.0f, -1.0f);

			Vertices[5 * 81 + 9 * i + j].Position = XMFLOAT3(-1.0f + j * 0.25f, -1.0f, -1.0f + i * 0.25f);
			Vertices[5 * 81 + 9 * i + j].TexCoord = XMFLOAT2(j * 0.125f, i * 0.125f);
			Vertices[5 * 81 + 9 * i + j].Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
			Vertices[5 * 81 + 9 * i + j].Tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
			Vertices[5 * 81 + 9 * i + j].Binormal = XMFLOAT3(0.0f, 0.0f, 1.0f);
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

	Texel *TexelData = new Texel[512 * 512 + 256 * 256 + 128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4];

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

	CompressedTexelBlockBC1 *CompressedTexelBlockDataBC1 = new CompressedTexelBlockBC1[128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1 * 1];

	CompressedTexelBlockBC1 *CompressedTexelBlocksBC1[8];

	CompressedTexelBlocksBC1[0] = CompressedTexelBlockDataBC1;
	CompressedTexelBlocksBC1[1] = CompressedTexelBlocksBC1[0] + 128 * 128;
	CompressedTexelBlocksBC1[2] = CompressedTexelBlocksBC1[1] + 64 * 64;
	CompressedTexelBlocksBC1[3] = CompressedTexelBlocksBC1[2] + 32 * 32;
	CompressedTexelBlocksBC1[4] = CompressedTexelBlocksBC1[3] + 16 * 16;
	CompressedTexelBlocksBC1[5] = CompressedTexelBlocksBC1[4] + 8 * 8;
	CompressedTexelBlocksBC1[6] = CompressedTexelBlocksBC1[5] + 4 * 4;
	CompressedTexelBlocksBC1[7] = CompressedTexelBlocksBC1[6] + 2 * 2;

	for (int k = 0; k < 8; k++)
	{
		int MIPSize = 128 >> k;

		for (int y = 0; y < MIPSize; y++)
		{
			for (int x = 0; x < MIPSize; x++)
			{
				Color MinColor{ 255, 255, 255 }, MaxColor{ 0, 0, 0 };

				float Distance = -1.0f;
				int j1max, i1max, j2max, i2max;

				for (int j1 = 0; j1 < 4; j1++)
				{
					for (int i1 = 0; i1 < 4; i1++)
					{
						Color color1{ (float)Texels[k][(4 * y + j1) * (4 * MIPSize) + (4 * x + i1)].R, (float)Texels[k][(4 * y + j1) * (4 * MIPSize) + (4 * x + i1)].G, (float)Texels[k][(4 * y + j1) * (4 * MIPSize) + (4 * x + i1)].B };

						for (int j2 = 0; j2 < 4; j2++)
						{
							for (int i2 = 0; i2 < 4; i2++)
							{
								Color color2{ (float)Texels[k][(4 * y + j2) * (4 * MIPSize) + (4 * x + i2)].R, (float)Texels[k][(4 * y + j2) * (4 * MIPSize) + (4 * x + i2)].G, (float)Texels[k][(4 * y + j2) * (4 * MIPSize) + (4 * x + i2)].B };

								float TestDistance = DistanceBetweenColor(color1, color2);

								if (TestDistance > Distance)
								{
									Distance = TestDistance;
									j1max = j1;
									i1max = i1;
									j2max = j2;
									i2max = i2;
								}
							}
						}
					}
				}

				MinColor = Color{ (float)Texels[k][(4 * y + j1max) * (4 * MIPSize) + (4 * x + i1max)].R, (float)Texels[k][(4 * y + j1max) * (4 * MIPSize) + (4 * x + i1max)].G, (float)Texels[k][(4 * y + j1max) * (4 * MIPSize) + (4 * x + i1max)].B };
				MaxColor = Color{ (float)Texels[k][(4 * y + j2max) * (4 * MIPSize) + (4 * x + i2max)].R, (float)Texels[k][(4 * y + j2max) * (4 * MIPSize) + (4 * x + i2max)].G, (float)Texels[k][(4 * y + j2max) * (4 * MIPSize) + (4 * x + i2max)].B };

				if ((MinColor.R < MaxColor.R) || ((MinColor.R == MinColor.R) && (MinColor.G < MaxColor.G)) || ((MinColor.R == MinColor.R) && (MinColor.G == MaxColor.G) && (MinColor.B < MaxColor.B)))
				{
					Color TmpColor = MinColor;
					MinColor = MaxColor;
					MaxColor = TmpColor;
				}

				Color Colors[4];
				Colors[0] = MaxColor;
				Colors[1] = MinColor;

				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[0] = 0;
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[1] = 0;

				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[0] |= ((((BYTE)((Colors[0].R / 255.0f) * 31.0f)) & 0b11111) << 11);
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[0] |= ((((BYTE)((Colors[0].G / 255.0f) * 63.0f)) & 0b111111) << 5);
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[0] |= ((((BYTE)((Colors[0].B / 255.0f) * 31.0f)) & 0b11111));

				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[1] |= ((((BYTE)((Colors[1].R / 255.0f) * 31.0f)) & 0b11111) << 11);
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[1] |= ((((BYTE)((Colors[1].G / 255.0f) * 63.0f)) & 0b111111) << 5);
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[1] |= ((((BYTE)((Colors[1].B / 255.0f) * 31.0f)) & 0b11111));

				CompressedTexelBlocksBC1[k][y * MIPSize + x].Texels[0] = 0;
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Texels[1] = 0;
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Texels[2] = 0;
				CompressedTexelBlocksBC1[k][y * MIPSize + x].Texels[3] = 0;


				if (CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[0] > CompressedTexelBlocksBC1[k][y * MIPSize + x].Colors[1])
				{
					Colors[2] = 2 * Colors[0] / 3 + Colors[1] / 3;
					Colors[3] = Colors[0] / 3 + 2 * Colors[1] / 3;
				}
				else
				{
					Colors[2] = Colors[0] / 2 + Colors[1] / 2;
					Colors[3] = Color{ 0.0f, 0.0f, 0.0f };
				}

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

						CompressedTexelBlocksBC1[k][y * MIPSize + x].Texels[j] |= ((ArgMin & 0b11) << (2 * i));
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
	texture2DResourceCreateInfo.CompressionType = BlockCompression::BC1;
	texture2DResourceCreateInfo.TexelData = (BYTE*)CompressedTexelBlockDataBC1;
	texture2DResourceCreateInfo.Width = 512;

	for (int k = 0; k < 4000; k++)
	{
		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);
	}

	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			Texels[0][y * 512 + x].R = 127;
			Texels[0][y * 512 + x].G = 127;
			Texels[0][y * 512 + x].B = 255;
			Texels[0][y * 512 + x].A = 255;
		}
	}

	for (int y = 128 - 28; y < 128 + 28; y++)
	{
		for (int x = y; x < 512 - y; x++)
		{
			Texels[0][y * 512 + x].R = 127;
			Texels[0][y * 512 + x].G = 37;
			Texels[0][y * 512 + x].B = 218;
			Texels[0][y * 512 + x].A = 255;
		}
	}

	for (int y = 256 + 128 - 28; y < 256 + 128 + 28; y++)
	{
		for (int x = 512 - y; x < y; x++)
		{
			Texels[0][y * 512 + x].R = 127;
			Texels[0][y * 512 + x].G = 218;
			Texels[0][y * 512 + x].B = 218;
			Texels[0][y * 512 + x].A = 255;
		}
	}

	for (int x = 128 - 28; x < 128 + 28; x++)
	{
		for (int y = x; y < 512 - x; y++)
		{
			Texels[0][y * 512 + x].R = 37;
			Texels[0][y * 512 + x].G = 127;
			Texels[0][y * 512 + x].B = 218;
			Texels[0][y * 512 + x].A = 255;
		}
	}

	for (int x = 256 + 128 - 28; x < 256 + 128 + 28; x++)
	{
		for (int y = 512 - x; y < x; y++)
		{
			Texels[0][y * 512 + x].R = 218;
			Texels[0][y * 512 + x].G = 127;
			Texels[0][y * 512 + x].B = 218;
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

	CompressedTexelBlockBC5 *CompressedTexelBlockDataBC5 = new CompressedTexelBlockBC5[128 * 128 + 64 * 64 + 32 * 32 + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2 + 1 * 1];

	CompressedTexelBlockBC5 *CompressedTexelBlocksBC5[8];

	CompressedTexelBlocksBC5[0] = CompressedTexelBlockDataBC5;
	CompressedTexelBlocksBC5[1] = CompressedTexelBlocksBC5[0] + 128 * 128;
	CompressedTexelBlocksBC5[2] = CompressedTexelBlocksBC5[1] + 64 * 64;
	CompressedTexelBlocksBC5[3] = CompressedTexelBlocksBC5[2] + 32 * 32;
	CompressedTexelBlocksBC5[4] = CompressedTexelBlocksBC5[3] + 16 * 16;
	CompressedTexelBlocksBC5[5] = CompressedTexelBlocksBC5[4] + 8 * 8;
	CompressedTexelBlocksBC5[6] = CompressedTexelBlocksBC5[5] + 4 * 4;
	CompressedTexelBlocksBC5[7] = CompressedTexelBlocksBC5[6] + 2 * 2;

	for (int k = 0; k < 8; k++)
	{
		int MIPSize = 128 >> k;

		for (int y = 0; y < MIPSize; y++)
		{
			for (int x = 0; x < MIPSize; x++)
			{
				float MinRed = 255.0f, MaxRed = 0.0f, MinGreen = 255.0f, MaxGreen = 0.0f;

				for (int j = 0; j < 4; j++)
				{
					for (int i = 0; i < 4; i++)
					{
						if ((float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R < MinRed) MinRed = (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R;
						if ((float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R > MaxRed) MaxRed = (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R;

						if ((float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G < MinGreen) MinGreen = (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G;
						if ((float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G > MaxGreen) MaxGreen = (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G;
					}
				}

				float RedColorTable[8], GreenColorTable[8];

				RedColorTable[0] = MaxRed;
				RedColorTable[1] = MinRed;

				GreenColorTable[0] = MaxGreen;
				GreenColorTable[1] = MinGreen;

				CompressedTexelBlocksBC5[k][y * MIPSize + x].Red[0] = (BYTE)RedColorTable[0];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].Red[1] = (BYTE)RedColorTable[1];

				CompressedTexelBlocksBC5[k][y * MIPSize + x].Green[0] = (BYTE)GreenColorTable[0];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].Green[1] = (BYTE)GreenColorTable[1];

				if (RedColorTable[0] > RedColorTable[1])
				{
					RedColorTable[2] = (6.0f * RedColorTable[0] + 1.0f * RedColorTable[1]) / 7.0f;
					RedColorTable[3] = (5.0f * RedColorTable[0] + 2.0f * RedColorTable[1]) / 7.0f;
					RedColorTable[4] = (4.0f * RedColorTable[0] + 3.0f * RedColorTable[1]) / 7.0f;
					RedColorTable[5] = (3.0f * RedColorTable[0] + 4.0f * RedColorTable[1]) / 7.0f;
					RedColorTable[6] = (2.0f * RedColorTable[0] + 5.0f * RedColorTable[1]) / 7.0f;
					RedColorTable[7] = (1.0f * RedColorTable[0] + 6.0f * RedColorTable[1]) / 7.0f;
				}
				else
				{
					RedColorTable[2] = (4.0f * RedColorTable[0] + 1.0f * RedColorTable[1]) / 5.0f;
					RedColorTable[3] = (3.0f * RedColorTable[0] + 2.0f * RedColorTable[1]) / 5.0f;
					RedColorTable[4] = (2.0f * RedColorTable[0] + 3.0f * RedColorTable[1]) / 5.0f;
					RedColorTable[5] = (1.0f * RedColorTable[0] + 4.0f * RedColorTable[1]) / 5.0f;
					RedColorTable[6] = 0.0f;
					RedColorTable[7] = 255.0f;
				}

				if (GreenColorTable[0] > GreenColorTable[1])
				{
					GreenColorTable[2] = (6.0f * GreenColorTable[0] + 1.0f * GreenColorTable[1]) / 7.0f;
					GreenColorTable[3] = (5.0f * GreenColorTable[0] + 2.0f * GreenColorTable[1]) / 7.0f;
					GreenColorTable[4] = (4.0f * GreenColorTable[0] + 3.0f * GreenColorTable[1]) / 7.0f;
					GreenColorTable[5] = (3.0f * GreenColorTable[0] + 4.0f * GreenColorTable[1]) / 7.0f;
					GreenColorTable[6] = (2.0f * GreenColorTable[0] + 5.0f * GreenColorTable[1]) / 7.0f;
					GreenColorTable[7] = (1.0f * GreenColorTable[0] + 6.0f * GreenColorTable[1]) / 7.0f;
				}
				else
				{
					GreenColorTable[2] = (4.0f * GreenColorTable[0] + 1.0f * GreenColorTable[1]) / 5.0f;
					GreenColorTable[3] = (3.0f * GreenColorTable[0] + 2.0f * GreenColorTable[1]) / 5.0f;
					GreenColorTable[4] = (2.0f * GreenColorTable[0] + 3.0f * GreenColorTable[1]) / 5.0f;
					GreenColorTable[5] = (1.0f * GreenColorTable[0] + 4.0f * GreenColorTable[1]) / 5.0f;
					GreenColorTable[6] = 0.0f;
					GreenColorTable[7] = 255.0f;
				}

				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[0] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[1] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[2] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[3] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[4] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[5] = 0;

				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[0] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[1] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[2] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[3] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[4] = 0;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[5] = 0;

				uint32_t RedIndices[2] = { 0, 0 }, GreenIndices[2] = { 0, 0 };

				size_t CurrentIndex = 0;

				for (int j = 0; j < 4; j++)
				{
					for (int i = 0; i < 4; i++)
					{
						Color TexelColor{ (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].R, (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].G, (float)Texels[k][(4 * y + j) * (4 * MIPSize) + (4 * x + i)].B };

						float RedDist = fabs(TexelColor.R - RedColorTable[0]);
						float GreenDist = fabs(TexelColor.G - GreenColorTable[0]);
						uint8_t RedArgMin = 0, GreenArgMin = 0;

						for (uint8_t x = 1; x < 8; x++)
						{
							float NewRedDist = fabs(TexelColor.R - RedColorTable[x]);
							float NewGreenDist = fabs(TexelColor.G - GreenColorTable[x]);

							if (NewRedDist < RedDist)
							{
								RedDist = NewRedDist;
								RedArgMin = x;
							}

							if (NewGreenDist < GreenDist)
							{
								GreenDist = NewGreenDist;
								GreenArgMin = x;
							}
						}

						RedIndices[CurrentIndex / 8] |= ((RedArgMin & 0b111) << (3 * (CurrentIndex % 8)));
						GreenIndices[CurrentIndex / 8] |= ((GreenArgMin & 0b111) << (3 * (CurrentIndex % 8)));

						CurrentIndex++;
					}
				}

				uint8_t *RedIndicesBytes[2] = { (uint8_t*)&RedIndices[0], (uint8_t*)&RedIndices[1] };
				uint8_t *GreenIndicesBytes[2] = { (uint8_t*)&GreenIndices[0], (uint8_t*)&GreenIndices[1] };
				
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[0] = RedIndicesBytes[0][0];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[1] = RedIndicesBytes[0][1];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[2] = RedIndicesBytes[0][2];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[3] = RedIndicesBytes[1][0];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[4] = RedIndicesBytes[1][1];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[5] = RedIndicesBytes[1][2];

				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[0] = GreenIndicesBytes[0][0];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[1] = GreenIndicesBytes[0][1];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[2] = GreenIndicesBytes[0][2];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[3] = GreenIndicesBytes[1][0];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[4] = GreenIndicesBytes[1][1];
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[5] = GreenIndicesBytes[1][2];

				/*CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[0] |= (RedIndices[0] & 0b111);
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[0] |= (RedIndices[1] & 0b111) << 3;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[0] |= (RedIndices[2] & 0b011) << 6;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[1] |= (RedIndices[2] & 0b100) >> 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[1] |= (RedIndices[3] & 0b111) << 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[1] |= (RedIndices[4] & 0b111) << 4;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[1] |= (RedIndices[5] & 0b001) << 7;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[2] |= (RedIndices[5] & 0b110) >> 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[2] |= (RedIndices[6] & 0b111) << 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[2] |= (RedIndices[7] & 0b111) << 5;

				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[3] |= (RedIndices[8] & 0b111);
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[3] |= (RedIndices[9] & 0b111) << 3;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[3] |= (RedIndices[10] & 0b011) << 6;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[4] |= (RedIndices[10] & 0b100) >> 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[4] |= (RedIndices[11] & 0b111) << 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[4] |= (RedIndices[12] & 0b111) << 4;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[4] |= (RedIndices[13] & 0b001) << 7;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[5] |= (RedIndices[13] & 0b110) >> 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[5] |= (RedIndices[14] & 0b111) << 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].RedIndices[5] |= (RedIndices[15] & 0b111) << 5;

				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[0] |= (GreenIndices[0] & 0b111);
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[0] |= (GreenIndices[1] & 0b111) << 3;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[0] |= (GreenIndices[2] & 0b001) << 6;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[1] |= (GreenIndices[2] & 0b110) >> 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[1] |= (GreenIndices[3] & 0b111) << 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[1] |= (GreenIndices[4] & 0b111) << 4;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[1] |= (GreenIndices[5] & 0b001) << 7;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[2] |= (GreenIndices[5] & 0b110) >> 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[2] |= (GreenIndices[6] & 0b111) << 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[2] |= (GreenIndices[7] & 0b111) << 5;

				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[3] |= (GreenIndices[8] & 0b111);
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[3] |= (GreenIndices[9] & 0b111) << 3;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[3] |= (GreenIndices[10] & 0b011) << 6;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[4] |= (GreenIndices[10] & 0b100) >> 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[4] |= (GreenIndices[11] & 0b111) << 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[4] |= (GreenIndices[12] & 0b111) << 4;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[4] |= (GreenIndices[13] & 0b001) << 7;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[5] |= (GreenIndices[13] & 0b110) >> 1;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[5] |= (GreenIndices[14] & 0b111) << 2;
				CompressedTexelBlocksBC5[k][y * MIPSize + x].GreenIndices[5] |= (GreenIndices[15] & 0b111) << 5;*/
			}
		}
	}

	texture2DResourceCreateInfo.Height = 512;
	texture2DResourceCreateInfo.MIPLevels = 8;
	texture2DResourceCreateInfo.SRGB = FALSE;
	texture2DResourceCreateInfo.Compressed = TRUE;
	texture2DResourceCreateInfo.CompressionType = BlockCompression::BC5;
	texture2DResourceCreateInfo.TexelData = (BYTE*)CompressedTexelBlockDataBC5;
	texture2DResourceCreateInfo.Width = 512;

	for (int k = 0; k < 4000; k++)
	{
		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Normal_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);
	}

	delete[] TexelData;
	delete[] CompressedTexelBlockDataBC1;
	delete[] CompressedTexelBlockDataBC5;

	HANDLE VertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ShaderModel51/MaterialBase_VertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER VertexShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(VertexShaderFile, &VertexShaderByteCodeLength);
	void *VertexShaderByteCodeData = malloc(VertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(VertexShaderFile, VertexShaderByteCodeData, (DWORD)VertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(VertexShaderFile);

	HANDLE PixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ShaderModel51/MaterialBase_PixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
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

	for (int k = 0; k < 4000; k++)
	{
		char MaterialResourceName[255];

		sprintf(MaterialResourceName, "Standart_%d", k);

		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);
		materialResourceCreateInfo.Textures[0] = Engine::GetEngine().GetResourceManager().GetResource<Texture2DResource>(Texture2DResourceName);
		sprintf(Texture2DResourceName, "Normal_%d", k);
		materialResourceCreateInfo.Textures[1] = Engine::GetEngine().GetResourceManager().GetResource<Texture2DResource>(Texture2DResourceName);

		Engine::GetEngine().GetResourceManager().AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);
	}

	free(VertexShaderByteCodeData);
	free(PixelShaderByteCodeData);

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