#pragma once

#include "CullingSubSystem.h"
#include "ClusterizationSubSystem.h"

struct RenderMesh
{
	VkBuffer MeshBuffer;
};

struct RenderTexture
{
	VkImage Texture;
	VkImageView TextureView;
};

struct RenderMaterial
{
	VkPipeline GBufferOpaquePassPipeline;
	VkPipeline ShadowMapPassPipeline;
};

#define SAFE_VK(Func) CheckVulkanCallResult(Func, u#Func);

enum class BlockCompression { BC1, BC2, BC3, BC4, BC5 };

struct RenderMeshCreateInfo
{
	void *MeshData;
	UINT VertexCount;
	UINT IndexCount;
};

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

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT2 TexCoord;
	XMFLOAT3 Normal;
	XMFLOAT3 Tangent;
	XMFLOAT3 Binormal;
};

struct Texel
{
	BYTE R, G, B, A;
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

	private:

	#if WITH_EDITOR
		void SetEditorViewportSize(const UINT Width, const UINT Height)
		{
			//renderDevice->SetEditorViewportSize(Width, Height);
		}
	#endif

		CullingSubSystem cullingSubSystem;
		ClusterizationSubSystem clusterizationSubSystem;

		int ResolutionWidth;
		int ResolutionHeight;

		VkInstance Instance;
		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;
		VkSurfaceKHR Surface;
		VkSwapchainKHR SwapChain;

#ifdef _DEBUG
		VkDebugUtilsMessengerEXT DebugUtilsMessenger;
#endif

		VkQueue CommandQueue;
		VkCommandPool CommandPool;
		VkCommandBuffer CommandBuffers[2];

		UINT CurrentBackBufferIndex, CurrentFrameIndex;

		VkSemaphore ImageAvailabilitySemaphore, ImagePresentationSemaphore;
		VkFence FrameSyncFences[2], CopySyncFence;

		VkDescriptorPool DescriptorPools[2];
		VkDescriptorSetLayout ConstantBuffersSetLayout, TexturesSetLayout, SamplersSetLayout;
		VkDescriptorSet SamplersSets[2], ConstantBuffersSets[2][100000], TexturesSets[2][100000];

		VkPipelineLayout PipelineLayout;

		VkImage *BackBufferTextures;
		VkImageView *BackBufferTexturesViews;

		uint32_t SwapChainImagesCount;

		// ===============================================================================================================

		VkSampler TextureSampler, ShadowMapSampler, BiLinearSampler, MinSampler;

		// ===============================================================================================================

		static const UINT MAX_MEMORY_HEAPS_COUNT = 200;
		static const SIZE_T BUFFER_MEMORY_HEAP_SIZE = 16 * 1024 * 1024, TEXTURE_MEMORY_HEAP_SIZE = 256 * 1024 * 1024;
		static const SIZE_T UPLOAD_HEAP_SIZE = 64 * 1024 * 1024;

		VkDeviceMemory BufferMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { VK_NULL_HANDLE }, TextureMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { VK_NULL_HANDLE };
		size_t BufferMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 }, TextureMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 };
		int CurrentBufferMemoryHeapIndex = 0, CurrentTextureMemoryHeapIndex = 0;

		VkDeviceMemory UploadHeap;
		VkBuffer UploadBuffer;
		size_t UploadBufferOffset = 0;

		uint32_t DefaultMemoryHeapIndex, UploadMemoryHeapIndex, ReadbackMemoryHeapIndex;

		// ===============================================================================================================

		VkImage GBufferTextures[2];
		VkImageView GBufferTexturesViews[2];

		VkImage DepthBufferTexture;
		VkImageView DepthBufferTextureView, DepthBufferTextureDepthReadView;

		VkRenderPass GBufferClearRenderPass, GBufferDrawRenderPass;

		VkFramebuffer GBufferFrameBuffer;

		VkBuffer GPUConstantBuffer, CPUConstantBuffers[2];

		VkDeviceMemory GPUMemory1, CPUMemory1;
		VkDeviceSize ConstantBuffersOffets1[2];

		// ===============================================================================================================

		VkImage ResolvedDepthBufferTexture;
		VkImageView ResolvedDepthBufferTextureView, ResolvedDepthBufferTextureDepthOnlyView;

		VkDeviceMemory GPUMemory2;

		VkRenderPass MSAADepthBufferResolveRenderPass;
		VkFramebuffer ResolvedDepthFrameBuffer;

		// ===============================================================================================================

		VkImage OcclusionBufferTexture;
		VkImageView  OcclusionBufferTextureView;
		VkBuffer OcclusionBufferReadbackBuffers[2];

		VkRenderPass OcclusionBufferRenderPass;
		VkFramebuffer OcclusionBufferFrameBuffer;

		VkPipeline OcclusionBufferPipeline;
		VkPipelineLayout OcclusionBufferPipelineLayout;
		VkDescriptorSetLayout OcclusionBufferSetLayout;
		VkDescriptorSet OcclusionBufferSets[2];

		VkDeviceMemory GPUMemory3, CPUMemory3;
		VkDeviceSize OcclusionBuffersOffsets[2];

		// ===============================================================================================================

		VkImage CascadedShadowMapTextures[4];
		VkImageView CascadedShadowMapTexturesViews[4];

		VkRenderPass ShadowMapClearRenderPass, ShadowMapDrawRenderPass;

		VkFramebuffer CascadedShadowMapFrameBuffers[4];

		VkBuffer GPUConstantBuffers2[4], CPUConstantBuffers2[4][2];

		VkDeviceMemory GPUMemory4, CPUMemory4;
		VkDeviceSize ConstantBuffersOffets2[4][2];

		// ===============================================================================================================

		VkImage ShadowMaskTexture;
		VkImageView ShadowMaskTextureView;

		VkRenderPass ShadowMaskRenderPass;
		VkFramebuffer ShadowMaskFrameBuffer;

		VkBuffer GPUShadowResolveConstantBuffer, CPUShadowResolveConstantBuffers[2];

		VkPipeline ShadowResolvePipeline;
		VkPipelineLayout ShadowResolvePipelineLayout;
		VkDescriptorSetLayout ShadowResolveSetLayout;
		VkDescriptorSet ShadowResolveSets[2];

		VkDeviceMemory GPUMemory5, CPUMemory5;
		VkDeviceSize ConstantBuffersOffets3[2];

		// ===============================================================================================================

		VkImage HDRSceneColorTexture;
		VkImageView HDRSceneColorTextureView;

		VkRenderPass DeferredLightingRenderPass;
		VkFramebuffer HDRSceneColorFrameBuffer;

		VkBuffer GPUDeferredLightingConstantBuffer, CPUDeferredLightingConstantBuffers[2];

		VkBuffer GPULightClustersBuffer, CPULightClustersBuffers[2];
		VkBufferView LightClustersBufferView;

		VkBuffer GPULightIndicesBuffer, CPULightIndicesBuffers[2];
		VkBufferView LightIndicesBufferView;

		VkBuffer GPUPointLightsBuffer, CPUPointLightsBuffers[2];

		VkPipeline DeferredLightingPipeline;
		VkPipelineLayout DeferredLightingPipelineLayout;
		VkDescriptorSetLayout DeferredLightingSetLayout;
		VkDescriptorSet DeferredLightingSets[2];

		VkDeviceMemory GPUMemory6, CPUMemory6;
		VkDeviceSize ConstantBuffersOffets4[2];
		VkDeviceSize DynamicBuffersOffsets[3][2];

		// ===============================================================================================================

		VkPipelineLayout SkyAndSunPipelineLayout;
		VkDescriptorSetLayout SkyAndSunSetLayout;
		VkDescriptorSet SkyAndSunSets[2][2];

		VkRenderPass SkyAndSunRenderPass;
		VkFramebuffer HDRSceneColorAndDepthFrameBuffer;

		VkBuffer SkyVertexBuffer, SkyIndexBuffer;
		VkBuffer GPUSkyConstantBuffer, CPUSkyConstantBuffers[2];
		VkPipeline SkyPipeline;
		VkImage SkyTexture;
		VkImageView SkyTextureView;

		VkBuffer SunVertexBuffer, SunIndexBuffer;
		VkBuffer GPUSunConstantBuffer, CPUSunConstantBuffers[2];
		VkPipeline SunPipeline;
		VkImage SunTexture;
		VkImageView SunTextureView;

		VkPipeline FogPipeline;
		VkPipelineLayout FogPipelineLayout;
		VkDescriptorSetLayout FogSetLayout;
		VkDescriptorSet FogSets[2];

		VkDeviceMemory GPUMemory7, CPUMemory7;
		VkDeviceSize ConstantBuffersOffets5[2][2];

		// ===============================================================================================================

		VkImage ResolvedHDRSceneColorTexture;
		VkImageView ResolvedHDRSceneColorTextureView;

		VkRenderPass HDRSceneColorResolveRenderPass;
		VkFramebuffer HDRSceneColorResolveFrameBuffer;

		VkDeviceMemory GPUMemory8;

		// ===============================================================================================================

		VkImage SceneLuminanceTextures[4];
		VkImageView SceneLuminanceTexturesViews[4];

		VkImage AverageLuminanceTexture;
		VkImageView AverageLuminanceTextureView;

		VkPipelineLayout LuminancePassPipelineLayout;
		VkDescriptorSetLayout LuminancePassSetLayout;
		VkDescriptorSet LuminancePassSets[5][2];

		VkPipeline LuminanceCalcPipeline;
		VkPipeline LuminanceSumPipeline;
		VkPipeline LuminanceAvgPipeline;

		VkDeviceMemory GPUMemory9;

		// ===============================================================================================================

		VkImage BloomTextures[3][7];
		VkImageView BloomTexturesViews[3][7];

		VkRenderPass BloomRenderPass;
		VkFramebuffer BloomTexturesFrameBuffers[3][7];

		VkPipelineLayout BloomPassPipelineLayout;
		VkDescriptorSetLayout BloomPassSetLayout;
		VkDescriptorSet BloomPassSets1[3][2], BloomPassSets2[6][3][2], BloomPassSets3[6][2];

		VkPipeline BrightPassPipeline;
		VkPipeline DownSamplePipeline;
		VkPipeline HorizontalBlurPipeline;
		VkPipeline VerticalBlurPipeline;
		VkPipeline UpSampleWithAddBlendPipeline;

		VkDeviceMemory GPUMemory10;

		// ===============================================================================================================

		VkImage ToneMappedImageTexture;
		VkImageView ToneMappedImageTextureView;

		VkRenderPass HDRToneMappingRenderPass;
		VkFramebuffer ToneMappedImageFrameBuffer;

		VkPipelineLayout HDRToneMappingPipelineLayout;
		VkDescriptorSetLayout HDRToneMappingSetLayout;
		VkDescriptorSet HDRToneMappingSets[2];

		VkPipeline HDRToneMappingPipeline;

		VkDeviceMemory GPUMemory11;

		// ===============================================================================================================

		VkRenderPass BackBufferResolveRenderPass;
		VkFramebuffer *BackBufferFrameBuffers;

		// ===============================================================================================================

		DynamicArray<RenderMesh*> RenderMeshDestructionQueue;
		DynamicArray<RenderMaterial*> RenderMaterialDestructionQueue;
		DynamicArray<RenderTexture*> RenderTextureDestructionQueue;

		inline void CheckVulkanCallResult(VkResult Result, const char16_t* Function);
		inline const char16_t* GetVulkanErrorMessageFromVkResult(VkResult Result);

		static const UINT MAX_MIP_LEVELS_IN_TEXTURE = 16;
};