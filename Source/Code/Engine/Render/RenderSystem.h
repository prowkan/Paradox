#pragma once

#include "CullingSubSystem.h"
#include "ClusterizationSubSystem.h"

#include <Containers/COMRCPtr.h>

struct RenderMesh
{
	COMRCPtr<ID3D12Resource> MeshBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS VertexBufferAddresses[3], IndexBufferAddress;
};

struct RenderTexture
{
	COMRCPtr<ID3D12Resource> Texture;
	D3D12_CPU_DESCRIPTOR_HANDLE TextureSRV;
};

struct RenderMaterial
{
	COMRCPtr<ID3D12PipelineState> GBufferOpaquePassPipelineState;
	COMRCPtr<ID3D12PipelineState> ShadowMapPassPipelineState;
};

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

#define SAFE_DX(Func) CheckDXCallResult(Func, u#Func);
#define UUIDOF(Value) __uuidof(Value), (void**)&Value

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

		void ToggleOcclusionBuffer()
		{
			DebugDrawOcclusionBuffer = !DebugDrawOcclusionBuffer;
		}

		void ToggleBoundingBoxes()
		{
			DebugDrawBoundingBoxes = !DebugDrawBoundingBoxes;
		}

	#if WITH_EDITOR
		void SetEditorViewportSize(const UINT Width, const UINT Height)
		{
			EditorViewportWidth = Width;
			EditorViewportHeight = Height;
		}
	#endif

	private:

		bool DebugDrawOcclusionBuffer = false;
		bool DebugDrawBoundingBoxes = false;

		CullingSubSystem cullingSubSystem;
		ClusterizationSubSystem clusterizationSubSystem;

		COMRCPtr<ID3D12Device> Device;
		COMRCPtr<IDXGISwapChain4> SwapChain;

		int ResolutionWidth;
		int ResolutionHeight;

		COMRCPtr<ID3D12CommandQueue> GraphicsCommandQueue, ComputeCommandQueue, CopyCommandQueue;
		COMRCPtr<ID3D12CommandAllocator> GraphicsCommandAllocators[2], ComputeCommandAllocators[2], CopyCommandAllocator;
		COMRCPtr<ID3D12GraphicsCommandList> GraphicsCommandList, ComputeCommandList, CopyCommandList;

		UINT CurrentBackBufferIndex, CurrentFrameIndex;

		COMRCPtr<ID3D12Fence> FrameSyncFences[2], CopySyncFence;
		HANDLE FrameSyncEvent, CopySyncEvent;

		COMRCPtr<ID3D12DescriptorHeap> RTDescriptorHeap, DSDescriptorHeap, CBSRUADescriptorHeap, SamplersDescriptorHeap;
		COMRCPtr<ID3D12DescriptorHeap> ConstantBufferDescriptorHeap, TexturesDescriptorHeap;
		COMRCPtr<ID3D12DescriptorHeap> FrameResourcesDescriptorHeaps[2], FrameSamplersDescriptorHeaps[2];

		UINT RTDescriptorsCount = 0, DSDescriptorsCount = 0, CBSRUADescriptorsCount = 0, SamplersDescriptorsCount = 0;
		UINT ConstantBufferDescriptorsCount = 0, TexturesDescriptorsCount = 0;

		COMRCPtr<ID3D12RootSignature> GraphicsRootSignature, ComputeRootSignature;

		COMRCPtr<ID3D12Resource> BackBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferTexturesRTVs[2];

		// ===============================================================================================================

		D3D12_CPU_DESCRIPTOR_HANDLE TextureSampler, ShadowMapSampler, BiLinearSampler, MinSampler;

		// ===============================================================================================================

		static const UINT MAX_MEMORY_HEAPS_COUNT = 200;
		static const SIZE_T BUFFER_MEMORY_HEAP_SIZE = 16 * 1024 * 1024, TEXTURE_MEMORY_HEAP_SIZE = 256 * 1024 * 1024;
		static const SIZE_T UPLOAD_HEAP_SIZE = 64 * 1024 * 1024;

		COMRCPtr<ID3D12Heap> BufferMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { nullptr }, TextureMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { nullptr };
		size_t BufferMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 }, TextureMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 };
		int CurrentBufferMemoryHeapIndex = 0, CurrentTextureMemoryHeapIndex = 0;

		COMRCPtr<ID3D12Heap> UploadHeap;
		COMRCPtr<ID3D12Resource> UploadBuffer;
		size_t UploadBufferOffset = 0;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> GPUCameraConstantBuffer, CPUCameraConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE CameraConstantBufferCBV;

		COMRCPtr<ID3D12Resource> GPURenderTargetConstantBuffer, CPURenderTargetConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetConstantBufferCBV;

		COMRCPtr<ID3D12Heap> GPUMemory0, CPUMemory0;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesRTVs[2], GBufferTexturesSRVs[2];

		COMRCPtr<ID3D12Resource> DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureDSV, DepthBufferTextureSRV;

		COMRCPtr<ID3D12Resource> GPUGBufferOpaquePassObjectsConstantBuffer, CPUGBufferOpaquePassObjectsConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferOpaquePassObjectsConstantBufferCBVs[20000];

		COMRCPtr<ID3D12Heap> GPUMemory1, CPUMemory1;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;

		COMRCPtr<ID3D12Heap> GPUMemory2;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> OcclusionBufferTexture, OcclusionBufferTextureReadback[2];
		D3D12_CPU_DESCRIPTOR_HANDLE OcclusionBufferTextureRTV;

		COMRCPtr<ID3D12PipelineState> OcclusionBufferPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory3, CPUMemory3;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesDSVs[4], CascadedShadowMapTexturesSRVs[4];

		COMRCPtr<ID3D12Resource> GPUShadowMapCameraConstantBuffer, CPUShadowMapCameraConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMapCameraConstantBufferCBVs[4];

		COMRCPtr<ID3D12Resource> GPUShadowMapPassObjectsConstantBuffers[4], CPUShadowMapPassObjectsConstantBuffers[4][2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMapPassObjectsConstantBufferCBVs[4][20000];

		COMRCPtr<ID3D12Heap> GPUMemory4, CPUMemory4;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureRTV, ShadowMaskTextureSRV;

		COMRCPtr<ID3D12Resource> GPUShadowResolveConstantBuffer, CPUShadowResolveConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowResolveConstantBufferCBV;

		COMRCPtr<ID3D12PipelineState> ShadowResolvePipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory5, CPUMemory5;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> HDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureRTV, HDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Resource> GPULightingConstantBuffer, CPULightingConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightingConstantBufferCBV;

		COMRCPtr<ID3D12Resource> GPUClusteredShadingConstantBuffer, CPUClusteredShadingConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ClusteredShadingConstantBufferCBV;

		COMRCPtr<ID3D12Resource> GPULightClustersBuffer, CPULightClustersBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightClustersBufferSRV;

		COMRCPtr<ID3D12Resource> GPULightIndicesBuffer, CPULightIndicesBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightIndicesBufferSRV;

		COMRCPtr<ID3D12Resource> GPUPointLightsBuffer, CPUPointLightsBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE PointLightsBufferSRV;

		COMRCPtr<ID3D12PipelineState> DeferredLightingPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory6, CPUMemory6;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> SkyVertexBuffer, SkyIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SkyVertexBufferAddress, SkyIndexBufferAddress;
		COMRCPtr<ID3D12Resource> GPUSkyConstantBuffer, CPUSkyConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE SkyConstantBufferCBV;
		COMRCPtr<ID3D12PipelineState> SkyPipelineState;
		COMRCPtr<ID3D12Resource> SkyTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SkyTextureSRV;

		COMRCPtr<ID3D12Resource> SunVertexBuffer, SunIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SunVertexBufferAddress, SunIndexBufferAddress;
		COMRCPtr<ID3D12Resource> GPUSunConstantBuffer, CPUSunConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE SunConstantBufferCBV;
		COMRCPtr<ID3D12PipelineState> SunPipelineState;
		COMRCPtr<ID3D12Resource> SunTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SunTextureSRV;

		COMRCPtr<ID3D12PipelineState> FogPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory7, CPUMemory7;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ResolvedHDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Heap> GPUMemory8;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> SceneLuminanceTextures[12];
		D3D12_CPU_DESCRIPTOR_HANDLE SceneLuminanceTexturesRTVs[12], SceneLuminanceTexturesSRVs[12];

		COMRCPtr<ID3D12Resource> AverageLuminanceTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE AverageLuminanceTextureRTV, AverageLuminanceTextureSRV;

		COMRCPtr<ID3D12PipelineState> LuminanceCalcPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceSumPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceAvgPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory9;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> BloomTextures[3][7];
		D3D12_CPU_DESCRIPTOR_HANDLE BloomTexturesRTVs[3][7], BloomTexturesSRVs[3][7];

		COMRCPtr<ID3D12PipelineState> BrightPassPipelineState;
		COMRCPtr<ID3D12PipelineState> DownSamplePipelineState;
		COMRCPtr<ID3D12PipelineState> HorizontalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> VerticalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> UpSampleWithAddBlendPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory10;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ToneMappedImageTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ToneMappedImageTextureRTV;

		COMRCPtr<ID3D12PipelineState> HDRToneMappingPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory11;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> DebugOcclusionBufferTexture, DebugOcclusionBufferTextureUpload[2];
		D3D12_CPU_DESCRIPTOR_HANDLE DebugOcclusionBufferTextureSRV;

		COMRCPtr<ID3D12PipelineState> DebugDrawOcclusionBufferPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory12, CPUMemory12;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> BoundingBoxIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS BoundingBoxIndexBufferAddress;

		COMRCPtr<ID3D12PipelineState> DebugDrawBoundingBoxPipelineState;

		COMRCPtr<ID3D12Resource> GPUConstantBuffer3, CPUConstantBuffers3[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs3[20000];

		COMRCPtr<ID3D12Heap> GPUMemory13, CPUMemory13;

		// ===============================================================================================================

		DynamicArray<RenderMesh*> RenderMeshDestructionQueue;
		DynamicArray<RenderMaterial*> RenderMaterialDestructionQueue;
		DynamicArray<RenderTexture*> RenderTextureDestructionQueue;

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		static const UINT MAX_MIP_LEVELS_IN_TEXTURE = 16;

	#if WITH_EDITOR
		UINT EditorViewportWidth;
		UINT EditorViewportHeight;
	#endif

		inline SIZE_T GetOffsetForResource(D3D12_RESOURCE_DESC& ResourceDesc, D3D12_HEAP_DESC& HeapDesc);
};