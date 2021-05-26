#pragma once

#include <Containers/COMRCPtr.h>
#include <Containers/DynamicArray.h>

#include "CullingSubSystem.h"
#include "ClusterizationSubSystem.h"

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

		void PrepareData();

	#if WITH_EDITOR
		void SetEditorViewportSize(const UINT Width, const UINT Height)
		{
			EditorViewportWidth = Width;
			EditorViewportHeight = Height;
		}
	#endif

		CullingSubSystem& GetCullingSubSystem() { return cullingSubSystem; }

	private:

		CullingSubSystem cullingSubSystem;
		ClusterizationSubSystem clusterizationSubSystem;

		COMRCPtr<ID3D12Device> Device;
		COMRCPtr<IDXGISwapChain4> SwapChain;

		int ResolutionWidth;
		int ResolutionHeight;

		COMRCPtr<ID3D12CommandQueue> CommandQueue;
		COMRCPtr<ID3D12CommandAllocator> CommandAllocators[2];
		COMRCPtr<ID3D12GraphicsCommandList> CommandList;

		UINT CurrentBackBufferIndex, CurrentFrameIndex;

		COMRCPtr<ID3D12Fence> FrameSyncFences[2], CopySyncFence;
		HANDLE FrameSyncEvent, CopySyncEvent;

		COMRCPtr<ID3D12DescriptorHeap> RTDescriptorHeap, DSDescriptorHeap, CBSRUADescriptorHeap, SamplersDescriptorHeap;
		COMRCPtr<ID3D12DescriptorHeap> ConstantBufferDescriptorHeap, TexturesDescriptorHeap;
		COMRCPtr<ID3D12DescriptorHeap> SceneResourcesDescriptorHeap, SceneSamplersDescriptorHeap;
		COMRCPtr<ID3D12DescriptorHeap> StaticResourcesDescriptorHeap, StaticSamplersDescriptorHeap;

		UINT RTDescriptorsCount = 0, DSDescriptorsCount = 0, CBSRUADescriptorsCount = 0, SamplersDescriptorsCount = 0;
		UINT ConstantBufferDescriptorsCount = 0, TexturesDescriptorsCount = 0;
		UINT StaticResourcesDescriptorsCount = 0, StaticSamplersDescriptorsCount = 0;

		COMRCPtr<ID3D12RootSignature> GraphicsRootSignature, ComputeRootSignature, SceneRenderRootSignature;

		COMRCPtr<ID3D12Resource> BackBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferTexturesRTVs[2];

		// ===============================================================================================================

		D3D12_CPU_DESCRIPTOR_HANDLE TextureSampler, ShadowMapSampler, BiLinearSampler, MinSampler;
		D3D12_GPU_DESCRIPTOR_HANDLE TextureSamplerDescriptorTable, ShadowMapSamplerDescriptorTable, BiLinearSamplerDescriptorTable, MinSamplerDescriptorTable;

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

		COMRCPtr<ID3D12Resource> GPUConstantBuffer, CPUConstantBuffers[2];

		D3D12_CPU_DESCRIPTOR_HANDLE CameraConstantBufferCBV;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowCameraConstantBufferCBVs[4];
		D3D12_CPU_DESCRIPTOR_HANDLE DeferredLightingConstantBufferCBV;
		D3D12_CPU_DESCRIPTOR_HANDLE SkyConstantBufferCBV;
		D3D12_CPU_DESCRIPTOR_HANDLE SunConstantBufferCBV;

		COMRCPtr<ID3D12Resource> GPUObjectsConstantBuffer, CPUObjectsConstantBuffers[2];

		D3D12_CPU_DESCRIPTOR_HANDLE ObjectsConstantBufferCBVs[20000];

		COMRCPtr<ID3D12Resource> GPUDrawDataConstantBuffer, CPUDrawDataConstantBuffers[2];

		COMRCPtr<ID3D12Heap> GPUMemory0, CPUMemory0;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesRTVs[2], GBufferTexturesSRVs[2];

		COMRCPtr<ID3D12Resource> DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureDSV, DepthBufferTextureSRV;

		COMRCPtr<ID3D12Heap> GPUMemory1;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;

		COMRCPtr<ID3D12Heap> GPUMemory2;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> OcclusionBufferTexture, OcclusionBufferTextureReadback[2];
		D3D12_CPU_DESCRIPTOR_HANDLE OcclusionBufferTextureRTV;

		COMRCPtr<ID3D12PipelineState> OcclusionBufferPipelineState;
		D3D12_GPU_DESCRIPTOR_HANDLE OcclusionBufferDescriptorTableSRV;

		COMRCPtr<ID3D12Heap> GPUMemory3, CPUMemory3;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesDSVs[4], CascadedShadowMapTexturesSRVs[4];

		COMRCPtr<ID3D12Heap> GPUMemory4;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureRTV, ShadowMaskTextureSRV;

		COMRCPtr<ID3D12PipelineState> ShadowResolvePipelineState;
		D3D12_GPU_DESCRIPTOR_HANDLE ShadowResolveDescriptorTableCBV, ShadowResolveDescriptorTableSRV;

		COMRCPtr<ID3D12Heap> GPUMemory5;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> HDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureRTV, HDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Resource> GPULightClustersBuffer, CPULightClustersBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightClustersBufferSRV;

		COMRCPtr<ID3D12Resource> GPULightIndicesBuffer, CPULightIndicesBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightIndicesBufferSRV;

		COMRCPtr<ID3D12Resource> GPUPointLightsBuffer, CPUPointLightsBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE PointLightsBufferSRV;

		COMRCPtr<ID3D12PipelineState> DeferredLightingPipelineState;
		D3D12_GPU_DESCRIPTOR_HANDLE DeferredLightingDescriptorTableCBV, DeferredLightingDescriptorTableSRV;

		COMRCPtr<ID3D12Heap> GPUMemory6, CPUMemory6;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> SkyVertexBuffer, SkyIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SkyVertexBufferAddress, SkyIndexBufferAddress;
		COMRCPtr<ID3D12PipelineState> SkyPipelineState;
		COMRCPtr<ID3D12Resource> SkyTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SkyTextureSRV;
		D3D12_GPU_DESCRIPTOR_HANDLE SkyDescriptorTableCBV, SkyDescriptorTableSRV;

		COMRCPtr<ID3D12Resource> SunVertexBuffer, SunIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SunVertexBufferAddress, SunIndexBufferAddress;
		COMRCPtr<ID3D12PipelineState> SunPipelineState;
		COMRCPtr<ID3D12Resource> SunTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SunTextureSRV;
		D3D12_GPU_DESCRIPTOR_HANDLE SunDescriptorTableCBV, SunDescriptorTableSRV;

		COMRCPtr<ID3D12PipelineState> FogPipelineState;
		D3D12_GPU_DESCRIPTOR_HANDLE FogDescriptorTableSRV;

		COMRCPtr<ID3D12Heap> GPUMemory7;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ResolvedHDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Heap> GPUMemory8;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> SceneLuminanceTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE SceneLuminanceTexturesUAVs[4], SceneLuminanceTexturesSRVs[4];

		COMRCPtr<ID3D12Resource> AverageLuminanceTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE AverageLuminanceTextureUAV, AverageLuminanceTextureSRV;

		COMRCPtr<ID3D12PipelineState> LuminanceCalcPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceSumPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceAvgPipelineState;
		D3D12_GPU_DESCRIPTOR_HANDLE SceneLuminanceDescriptorTablesSRVs[5], SceneLuminanceDescriptorTablesUAVs[5];

		COMRCPtr<ID3D12Heap> GPUMemory9;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> BloomTextures[3][7];
		D3D12_CPU_DESCRIPTOR_HANDLE BloomTexturesRTVs[3][7], BloomTexturesSRVs[3][7];

		COMRCPtr<ID3D12PipelineState> BrightPassPipelineState;
		COMRCPtr<ID3D12PipelineState> DownSamplePipelineState;
		COMRCPtr<ID3D12PipelineState> HorizontalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> VerticalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> UpSampleWithAddBlendPipelineState;
		D3D12_GPU_DESCRIPTOR_HANDLE BloomDescriptorTablesSRVs[3 + 6 * 3 + 6];

		COMRCPtr<ID3D12Heap> GPUMemory10;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> ToneMappedImageTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ToneMappedImageTextureRTV;

		COMRCPtr<ID3D12PipelineState> HDRToneMappingPipelineState;
		D3D12_GPU_DESCRIPTOR_HANDLE HDRToneMappingDescriptorTableSRV;

		COMRCPtr<ID3D12Heap> GPUMemory11;

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
};