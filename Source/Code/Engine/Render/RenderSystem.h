#pragma once

#include "CullingSubSystem.h"
#include "ClusterizationSubSystem.h"

#include "RenderDevice.h"

struct RenderMesh;
struct RenderTexture;
struct RenderMaterial;

struct RenderMeshCreateInfo
{
	void *MeshData;
	UINT VertexCount;
	UINT IndexCount;
};

enum class BlockCompression { BC1, BC2, BC3, BC4, BC5 };

struct RenderTextureCreateInfo
{
	UINT Width, Height;
	UINT MIPLevels;
	BOOL SRGB;
	BOOL Compressed;
	BlockCompression CompressionType;
	BYTE *TexelData;
};

struct RenderMaterialCreateInfo
{
	void *GBufferOpaquePassVertexShaderByteCodeData;
	void *GBufferOpaquePassPixelShaderByteCodeData;
	size_t GBufferOpaquePassVertexShaderByteCodeLength;
	size_t GBufferOpaquePassPixelShaderByteCodeLength;
	void *ShadowMapPassVertexShaderByteCodeData;
	void *ShadowMapPassPixelShaderByteCodeData;
	size_t ShadowMapPassVertexShaderByteCodeLength;
	size_t ShadowMapPassPixelShaderByteCodeLength;
};

class RenderSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

		RenderMesh* CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo);
		RenderTexture* CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo);
		RenderMaterial* CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo);

		void DestroyRenderMesh(RenderMesh* renderMesh);
		void DestroyRenderTexture(RenderTexture* renderTexture);
		void DestroyRenderMaterial(RenderMaterial* renderMaterial);

		CullingSubSystem& GetCullingSubSystem() { return cullingSubSystem; }
		ClusterizationSubSystem& GetClusterizationSubSystem() { return clusterizationSubSystem; }

	#if WITH_EDITOR
		void SetEditorViewportSize(const UINT Width, const UINT Height)
		{
			renderDevice->SetEditorViewportSize(Width, Height);
		}
	#endif

	private:

		RenderDevice *renderDevice;

		CullingSubSystem cullingSubSystem;
		ClusterizationSubSystem clusterizationSubSystem;
};