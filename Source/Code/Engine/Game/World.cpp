#include "World.h"

#include "MetaClass.h"
#include "GameObject.h"

#include "GameObjects/Render/Meshes/StaticMeshObject.h"

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

	Texture2DResourceCreateInfo texture2DResourceCreateInfo;
	texture2DResourceCreateInfo.Height = 512;
	texture2DResourceCreateInfo.MIPLevels = 8;
	texture2DResourceCreateInfo.SRGB = TRUE;
	texture2DResourceCreateInfo.TexelData = (BYTE*)TexelData;
	texture2DResourceCreateInfo.Width = 512;

	for (int k = 0; k < 4000; k++)
	{
		char Texture2DResourceName[255];

		sprintf(Texture2DResourceName, "Checker_%d", k);

		Engine::GetEngine().GetResourceManager().AddResource<Texture2DResource>(Texture2DResourceName, &texture2DResourceCreateInfo);
	}

	delete[] TexelData;

	const char *VertexShaderSourceCode = R"(
	
			struct VSInput
			{
				float3 Position : POSITION;
				float2 TexCoord : TEXCOORD;
			};

			struct VSOutput
			{
				float4 Position : SV_Position;
				float2 TexCoord : TEXCOORD;
			};

			struct VSConstants
			{
				float4x4 WVPMatrix;
			};

			ConstantBuffer<VSConstants> VertexShaderConstants : register(b0);

			VSOutput VS(VSInput VertexShaderInput)
			{
				VSOutput VertexShaderOutput;

				VertexShaderOutput.Position = mul(float4(VertexShaderInput.Position, 1.0f), VertexShaderConstants.WVPMatrix);
				VertexShaderOutput.TexCoord = VertexShaderInput.TexCoord;

				return VertexShaderOutput;
			}

		)";

	const char *PixelShaderSourceCode = R"(

			struct PSInput
			{
				float4 Position : SV_Position;
				float2 TexCoord : TEXCOORD;
			};

			Texture2D Texture : register(t0);
			SamplerState Sampler : register(s0);

			float4 PS(PSInput PixelShaderInput) : SV_Target
			{
				return float4(Texture.Sample(Sampler, PixelShaderInput.TexCoord).rgb, 1.0f);
			}

		)";

	COMRCPtr<ID3DBlob> ErrorBlob;

	COMRCPtr<ID3DBlob> VertexShaderBlob, PixelShaderBlob;

	HRESULT hr;

	hr = D3DCompile(VertexShaderSourceCode, strlen(VertexShaderSourceCode), "VertexShader", nullptr, nullptr, "VS", "vs_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &VertexShaderBlob, &ErrorBlob);
	hr = D3DCompile(PixelShaderSourceCode, strlen(PixelShaderSourceCode), "PixelShader", nullptr, nullptr, "PS", "ps_5_1", D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, &PixelShaderBlob, &ErrorBlob);

	MaterialResourceCreateInfo materialResourceCreateInfo;
	materialResourceCreateInfo.PixelShaderByteCodeData = PixelShaderBlob->GetBufferPointer();
	materialResourceCreateInfo.PixelShaderByteCodeLength = (UINT)PixelShaderBlob->GetBufferSize();
	materialResourceCreateInfo.VertexShaderByteCodeData = VertexShaderBlob->GetBufferPointer();
	materialResourceCreateInfo.VertexShaderByteCodeLength = (UINT)VertexShaderBlob->GetBufferSize();
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

			sprintf(StaticMeshResourceName, "Cube_%d", ResourceCounter);
			sprintf(MaterialResourceName, "Standart_%d", ResourceCounter);

			StaticMeshObject *staticMeshObject = GameObject::DynamicCast<StaticMeshObject>(SpawnGameObject(StaticMeshObject::GetMetaClassStatic()));
			staticMeshObject->GetTransformComponent()->SetLocation(XMFLOAT3(i * 5.0f + 2.5f, -0.0f, j * 5.0f + 2.5f));
			staticMeshObject->GetStaticMeshComponent()->SetStaticMesh(Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshObject->GetStaticMeshComponent()->SetMaterial(Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;

			sprintf(StaticMeshResourceName, "Cube_%d", ResourceCounter);
			sprintf(MaterialResourceName, "Standart_%d", ResourceCounter);

			staticMeshObject = GameObject::DynamicCast<StaticMeshObject>(SpawnGameObject(StaticMeshObject::GetMetaClassStatic()));
			staticMeshObject->GetTransformComponent()->SetLocation(XMFLOAT3(i * 10.0f + 5.0f, -2.0f, j * 10.0f + 5.0f));
			staticMeshObject->GetTransformComponent()->SetScale(XMFLOAT3(5.0f, 1.0f, 5.0f));
			staticMeshObject->GetStaticMeshComponent()->SetStaticMesh(Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName));
			staticMeshObject->GetStaticMeshComponent()->SetMaterial(Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName));

			ResourceCounter = (ResourceCounter + 1) % 4000;
		}
	}
}

void World::UnLoadWorld()
{
	Engine::GetEngine().GetResourceManager().DestroyAllResources();
}

GameObject* World::SpawnGameObject(MetaClass* metaClass)
{
	void *gameObjectPtr = Engine::GetEngine().GetMemoryManager().AllocateGameObject(metaClass);
	metaClass->ObjectConstructorFunc(gameObjectPtr);
	GameObject *gameObject = (GameObject*)gameObjectPtr;
	gameObject->SetMetaClass(metaClass);
	gameObject->SetWorld(this);
	gameObject->InitDefaultProperties();
	GameObjects.push_back(gameObject);
	return gameObject;
}


void World::TickWorld(float DeltaTime)
{

}