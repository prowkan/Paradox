#pragma once

#include "CullingSubSystem.h"

struct RenderMesh
{
	VkBuffer VertexBuffer, IndexBuffer;
};

struct RenderTexture
{
	VkImage Texture;
	VkImageView TextureView;
};

struct RenderMaterial
{
	VkPipeline Pipeline;
};

struct RenderMeshCreateInfo
{
	UINT VertexCount;
	void *VertexData;
	UINT IndexCount;
	void *IndexData;
};

struct RenderTextureCreateInfo
{
	UINT Width, Height;
	UINT MIPLevels;
	BOOL SRGB;
	BYTE *TexelData;
};

struct RenderMaterialCreateInfo
{
	UINT VertexShaderByteCodeLength;
	void *VertexShaderByteCodeData;
	UINT PixelShaderByteCodeLength;
	void *PixelShaderByteCodeData;
};

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT2 TexCoord;
};

struct Texel
{
	BYTE R, G, B, A;
};

#define SAFE_VK(Func) CheckVulkanCallResult(Func, L#Func);

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

	private:

		VkInstance Instance;
		VkPhysicalDevice PhysicalDevice;
		VkDevice Device;
		VkSurfaceKHR Surface;
		VkSwapchainKHR SwapChain;

#ifdef _DEBUG
		VkDebugUtilsMessengerEXT DebugUtilsMessenger;
#endif

		int ResolutionWidth;
		int ResolutionHeight;

		VkQueue CommandQueue;
		VkCommandPool CommandPools[2];
		VkCommandBuffer CommandBuffers[2];

		UINT CurrentBackBufferIndex, CurrentFrameIndex;

		VkSemaphore ImageAvailabilitySemaphore, ImagePresentationSemaphore;
		VkFence Fences[2];
		
		VkDescriptorPool DescriptorPools[2];
		VkDescriptorSetLayout ConstantBuffersSetLayout, TexturesSetLayout, SamplersSetLayout;
		VkDescriptorSet SamplersSets[2], ConstantBuffersSets[2][20000], TexturesSets[2][20000];

		VkPipelineLayout PipelineLayout;

		VkImage BackBufferTextures[2];
		VkImageView BackBufferRTVs[2];

		VkImage DepthBufferTexture;
		VkImageView DepthBufferDSV;
		VkDeviceMemory DepthBufferTextureMemoryHeap;

		VkRenderPass RenderPass;
		VkFramebuffer FrameBuffers[2];

		VkBuffer GPUConstantBuffer, CPUConstantBuffers[2];
		VkDeviceMemory GPUConstantBufferMemoryHeap, CPUConstantBufferMemoryHeaps[2];

		VkSampler Sampler;

		static const UINT MAX_MEMORY_HEAPS_COUNT = 200;
		static const SIZE_T BUFFER_MEMORY_HEAP_SIZE = 16 * 1024 * 1024, TEXTURE_MEMORY_HEAP_SIZE = 256 * 1024 * 1024;
		static const SIZE_T UPLOAD_HEAP_SIZE = 64 * 1024 * 1024;

		VkDeviceMemory BufferMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { VK_NULL_HANDLE }, TextureMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { VK_NULL_HANDLE };
		size_t BufferMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 }, TextureMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 };
		int CurrentBufferMemoryHeapIndex = 0, CurrentTextureMemoryHeapIndex = 0;

		VkDeviceMemory UploadHeap;
		VkBuffer UploadBuffer;
		size_t UploadBufferOffset = 0;

		vector<RenderMesh*> RenderMeshDestructionQueue;
		vector<RenderMaterial*> RenderMaterialDestructionQueue;
		vector<RenderTexture*> RenderTextureDestructionQueue;

		CullingSubSystem cullingSubSystem;

		inline void CheckVulkanCallResult(VkResult Result, const wchar_t* Function);
		inline const wchar_t* GetVulkanErrorMessageFromVkResult(VkResult Result);
};