// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "StaticMeshComponent.h"

#include "../../../Entity.h"

#include "../../Common/TransformComponent.h"
#include "../../Common/BoundingBoxComponent.h"

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

#include <Engine/Engine.h>

#include <FileSystem/LevelFile.h>

#include <Containers/Pointer.h>

DEFINE_METACLASS_VARIABLE(StaticMeshComponent)

void StaticMeshComponent::InitComponentDefaultProperties()
{
	transformComponent = Owner->GetComponent<TransformComponent>();
	boundingBoxComponent = Owner->GetComponent<BoundingBoxComponent>();
}

void StaticMeshComponent::RegisterComponent()
{
	Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().RegisterStaticMeshComponent(this);
}

void StaticMeshComponent::UnRegisterComponent()
{
}

void StaticMeshComponent::LoadFromFile(LevelFile& File)
{
	String StaticMeshResourceName = File.Read<String>();
	
	if (!Engine::GetEngine().GetResourceManager().IsResourceLoaded(StaticMeshResourceName))
	{
		struct StaticMeshFileHeader
		{
			UINT VertexCount;
			UINT IndexCount;
		};

		Pointer<BYTE[]> StaticMeshData = Pointer<BYTE[]>::Create(Engine::GetEngine().GetFileSystem().GetFileSize(StaticMeshResourceName));
		Engine::GetEngine().GetFileSystem().LoadFile(StaticMeshResourceName, StaticMeshData);

		StaticMeshFileHeader *staticMeshFileHeader = (StaticMeshFileHeader*)((BYTE*)StaticMeshData + 2);

		StaticMeshResourceCreateInfo staticMeshResourceCreateInfo;
		staticMeshResourceCreateInfo.VertexCount = staticMeshFileHeader->VertexCount;
		staticMeshResourceCreateInfo.IndexCount = staticMeshFileHeader->IndexCount;
		staticMeshResourceCreateInfo.MeshData = (BYTE*)staticMeshFileHeader + sizeof(StaticMeshFileHeader);

		Engine::GetEngine().GetResourceManager().AddResource<StaticMeshResource>(StaticMeshResourceName, &staticMeshResourceCreateInfo);
	}

	StaticMesh = Engine::GetEngine().GetResourceManager().GetResource<StaticMeshResource>(StaticMeshResourceName);
	
	for (UINT i = 0; i < StaticMesh->GetTotalElementsCount(); i++)
	{
		String MaterialResourceName = File.Read<String>();

		if (!Engine::GetEngine().GetResourceManager().IsResourceLoaded(MaterialResourceName))
		{
			Pointer<BYTE[]> MaterialData = Pointer<BYTE[]>::Create(Engine::GetEngine().GetFileSystem().GetFileSize(MaterialResourceName));
			Engine::GetEngine().GetFileSystem().LoadFile(MaterialResourceName, MaterialData);

#pragma pack(push, 1)

			struct MaterialFileHeader
			{
				BYTE BlendMode;
				USHORT TexturesCount;
			};

#pragma pack(pop, 1)

			MaterialFileHeader* materialFileHeader = (MaterialFileHeader*)((BYTE*)MaterialData + 2);

			const char* ShaderModel = "ShaderModel60";

			MaterialResourceCreateInfo materialResourceCreateInfo;

			if (materialFileHeader->BlendMode == 0)
			{
				char ShaderFileName[255];
				sprintf(ShaderFileName, "%s.%s.GBufferOpaquePass_VertexShader", ShaderModel, MaterialResourceName.GetData());

				void* GBufferOpaquePassVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
				SIZE_T GBufferOpaquePassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

				sprintf(ShaderFileName, "%s.%s.GBufferOpaquePass_PixelShader", ShaderModel, MaterialResourceName.GetData());

				void* GBufferOpaquePassPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
				SIZE_T GBufferOpaquePassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

				sprintf(ShaderFileName, "%s.%s.ShadowMapPass_VertexShader", ShaderModel, MaterialResourceName.GetData());

				void* ShadowMapPassVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
				SIZE_T ShadowMapPassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

				materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeData = GBufferOpaquePassPixelShaderByteCodeData;
				materialResourceCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength = GBufferOpaquePassPixelShaderByteCodeLength;
				materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeData = GBufferOpaquePassVertexShaderByteCodeData;
				materialResourceCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength = GBufferOpaquePassVertexShaderByteCodeLength;
				materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeData = nullptr;
				materialResourceCreateInfo.ShadowMapPassPixelShaderByteCodeLength = 0;
				materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeData = ShadowMapPassVertexShaderByteCodeData;
				materialResourceCreateInfo.ShadowMapPassVertexShaderByteCodeLength = ShadowMapPassVertexShaderByteCodeLength;
			}
			else if(materialFileHeader->BlendMode == 1)
			{
				char ShaderFileName[255];
				sprintf(ShaderFileName, "%s.%s.TransparentPass_VertexShader", ShaderModel, MaterialResourceName.GetData());

				void* TransparentPassVertexShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
				SIZE_T TransparentPassVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

				sprintf(ShaderFileName, "%s.%s.TransparentPass_PixelShader", ShaderModel, MaterialResourceName.GetData());

				void* TransparentPassPixelShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData(ShaderFileName);
				SIZE_T TransparentPassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize(ShaderFileName);

				materialResourceCreateInfo.TransparentPassPixelShaderByteCodeData = TransparentPassPixelShaderByteCodeData;
				materialResourceCreateInfo.TransparentPassPixelShaderByteCodeLength = TransparentPassPixelShaderByteCodeLength;
				materialResourceCreateInfo.TransparentPassVertexShaderByteCodeData = TransparentPassVertexShaderByteCodeData;
				materialResourceCreateInfo.TransparentPassVertexShaderByteCodeLength = TransparentPassVertexShaderByteCodeLength;
			}		
			
			materialResourceCreateInfo.Textures.Resize(materialFileHeader->TexturesCount);
			materialResourceCreateInfo.BlendMode = materialFileHeader->BlendMode;

			SIZE_T Offset = 0;

			for (USHORT i = 0; i < materialFileHeader->TexturesCount; i++)
			{
				String TextureName((char*)((BYTE*)MaterialData + 5 + Offset));

				Offset += TextureName.GetLength() + 1;

				if (!Engine::GetEngine().GetResourceManager().IsResourceLoaded(TextureName))
				{
					struct TextureFileHeader
					{
						UINT Width;
						UINT Height;
						UINT MIPLevels;
						BOOL SRGB;
						BOOL Compressed;
					};

					Pointer<BYTE[]> TextureData = Pointer<BYTE[]>::Create(Engine::GetEngine().GetFileSystem().GetFileSize(TextureName));
					Engine::GetEngine().GetFileSystem().LoadFile(TextureName, TextureData);

					TextureFileHeader* textureFileHeader = (TextureFileHeader*)((BYTE*)TextureData + 2);

					Texture2DResourceCreateInfo texture2DResourceCreateInfo;
					texture2DResourceCreateInfo.Height = textureFileHeader->Height;
					texture2DResourceCreateInfo.MIPLevels = textureFileHeader->MIPLevels;
					texture2DResourceCreateInfo.SRGB = textureFileHeader->SRGB;
					texture2DResourceCreateInfo.Compressed = textureFileHeader->Compressed;
					texture2DResourceCreateInfo.CompressionType = i == 1 ? BlockCompression::BC5 : BlockCompression::BC1;
					texture2DResourceCreateInfo.TexelData = (BYTE*)textureFileHeader + sizeof(TextureFileHeader);
					texture2DResourceCreateInfo.Width = textureFileHeader->Width;

					Engine::GetEngine().GetResourceManager().AddResource<Texture2DResource>(TextureName, &texture2DResourceCreateInfo);
				}

				materialResourceCreateInfo.Textures[i] = Engine::GetEngine().GetResourceManager().GetResource<Texture2DResource>(TextureName);
			}

			Engine::GetEngine().GetResourceManager().AddResource<MaterialResource>(MaterialResourceName, &materialResourceCreateInfo);
		}

		Materials.Add(Engine::GetEngine().GetResourceManager().GetResource<MaterialResource>(MaterialResourceName));
	}

	transformComponent = Owner->GetComponent<TransformComponent>();
	boundingBoxComponent = Owner->GetComponent<BoundingBoxComponent>();
}