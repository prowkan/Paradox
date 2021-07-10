#pragma once

#include "CullingSubSystem.h"
#include "ClusterizationSubSystem.h"

#include <Containers/COMRCPtr.h>
#include <Containers/Pointer.h>

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

#define SAFE_DX(Func) RenderSystem::CheckDXCallResult(Func, u#Func);
#define UUIDOF(Value) __uuidof(Value), (void**)&Value

class DescriptorHeap
{
	public:

		DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE DescriptorsType, UINT DescriptorsCount, ID3D12Device* Device, const char16_t* DebugHeapName = nullptr)
		{
			D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
			DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			DescriptorHeapDesc.NodeMask = 0;
			DescriptorHeapDesc.NumDescriptors = DescriptorsCount;
			DescriptorHeapDesc.Type = DescriptorsType;

			HRESULT hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(DXDescriptorHeap));
			if (DebugHeapName) hr = DXDescriptorHeap->SetName((LPCWSTR)DebugHeapName);

			FirstDescriptor = DXDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			DescriptorSize = Device->GetDescriptorHandleIncrementSize(DescriptorsType);
			AllocatedDescriptorsCount = 0;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor()
		{
			D3D12_CPU_DESCRIPTOR_HANDLE Descriptor;
			Descriptor.ptr = FirstDescriptor + AllocatedDescriptorsCount * DescriptorSize;
			AllocatedDescriptorsCount++;

			return Descriptor;
		}

	private:

		COMRCPtr<ID3D12DescriptorHeap> DXDescriptorHeap;

		SIZE_T FirstDescriptor;
		SIZE_T DescriptorSize;
		UINT AllocatedDescriptorsCount;
};

class DescriptorTable;

class FrameDescriptorHeap
{
	public:

		FrameDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE DescriptorsType, UINT DescriptorsCount, ID3D12Device* Device, const char16_t* DebugHeapName = nullptr)
		{
			D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc;
			DescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			DescriptorHeapDesc.NodeMask = 0;
			DescriptorHeapDesc.NumDescriptors = DescriptorsCount;
			DescriptorHeapDesc.Type = DescriptorsType;

			this->DescriptorsType = DescriptorsType;

			HRESULT hr = Device->CreateDescriptorHeap(&DescriptorHeapDesc, UUIDOF(DXDescriptorHeap));
			if (DebugHeapName) hr = DXDescriptorHeap->SetName((LPCWSTR)DebugHeapName);

			FirstCPUDescriptor = DXDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			FirstGPUDescriptor = DXDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;
			DescriptorSize = Device->GetDescriptorHandleIncrementSize(DescriptorsType);

			Descriptors = (D3D12_CPU_DESCRIPTOR_HANDLE*)SystemMemoryAllocator::AllocateMemory(sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * DescriptorsCount);
			ArrayOfOnes = (UINT*)SystemMemoryAllocator::AllocateMemory(sizeof(UINT) * DescriptorsCount);

			for (UINT i = 0; i < DescriptorsCount; i++)
			{
				ArrayOfOnes[i] = 1;
			}

			AllocatedDescriptorsCount = 0;
		}

		void Reset()
		{
			AllocatedDescriptorsCount = 0;
		}

		DescriptorTable AllocateDescriptorTable(UINT DescriptorsCount);

		ID3D12DescriptorHeap* GetDescriptorHeap() { return DXDescriptorHeap; }

		D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType() { return DescriptorsType; }

	private:

		COMRCPtr<ID3D12DescriptorHeap> DXDescriptorHeap;

		D3D12_DESCRIPTOR_HEAP_TYPE DescriptorsType;

		SIZE_T FirstCPUDescriptor;
		UINT64 FirstGPUDescriptor;

		UINT DescriptorSize;

		D3D12_CPU_DESCRIPTOR_HANDLE *Descriptors;
		UINT *ArrayOfOnes;

		UINT AllocatedDescriptorsCount;
};

class DescriptorTable
{
	friend class FrameDescriptorHeap;

	public:

		DescriptorTable()
		{

		}

		DescriptorTable(const DescriptorTable& OtherTable)
		{
			TableSize = OtherTable.TableSize;
			Descriptors = OtherTable.Descriptors;
			FirstCPUDescriptor = OtherTable.FirstCPUDescriptor;
			FirstGPUDescriptor = OtherTable.FirstGPUDescriptor;
			ParentDescriptorHeap = OtherTable.ParentDescriptorHeap;
			RangesArray = OtherTable.RangesArray;
		}

		void SetConstantBuffer(UINT SlotIndex, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
		{
			Descriptors[SlotIndex] = Descriptor;
		}

		void SetBuffer(UINT SlotIndex, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
		{
			Descriptors[SlotIndex] = Descriptor;
		}

		void SetRWBuffer(UINT SlotIndex, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
		{
			Descriptors[SlotIndex] = Descriptor;
		}

		void SetTexture(UINT SlotIndex, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
		{
			Descriptors[SlotIndex] = Descriptor;
		}

		void SetRWTexture(UINT SlotIndex, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
		{
			Descriptors[SlotIndex] = Descriptor;
		}

		void SetSampler(UINT SlotIndex, D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
		{
			Descriptors[SlotIndex] = Descriptor;
		}

		void UpdateTable(ID3D12Device* Device)
		{
			Device->CopyDescriptors(1, &FirstCPUDescriptor, &TableSize, TableSize, Descriptors, RangesArray, ParentDescriptorHeap->GetDescriptorHeapType());
		}

		operator D3D12_GPU_DESCRIPTOR_HANDLE()
		{
			return FirstGPUDescriptor;
		}		

	private:
		
		UINT TableSize;

		D3D12_CPU_DESCRIPTOR_HANDLE *Descriptors;
		UINT *RangesArray;

		D3D12_CPU_DESCRIPTOR_HANDLE FirstCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE FirstGPUDescriptor;

		FrameDescriptorHeap *ParentDescriptorHeap;
};

inline DescriptorTable FrameDescriptorHeap::AllocateDescriptorTable(UINT TableSize)
{
	DescriptorTable Table;

	Table.Descriptors = Descriptors + AllocatedDescriptorsCount;
	Table.FirstCPUDescriptor.ptr = FirstCPUDescriptor + AllocatedDescriptorsCount * DescriptorSize;
	Table.FirstGPUDescriptor.ptr = FirstGPUDescriptor + AllocatedDescriptorsCount * DescriptorSize;
	Table.ParentDescriptorHeap = this;
	Table.RangesArray = ArrayOfOnes + AllocatedDescriptorsCount;
	Table.TableSize = TableSize;

	AllocatedDescriptorsCount += TableSize;

	return Table;
}

class Buffer
{
	friend class RenderSystem;

	public:

	private:
		
		COMRCPtr<ID3D12Resource> DXBuffer;

		D3D12_RESOURCE_STATES BufferState;
};

class Texture
{
	friend class RenderSystem;

	public:

	private:

		COMRCPtr<ID3D12Resource> DXTexture;

		D3D12_RESOURCE_STATES *TextureSubResourceStates;
		UINT SubResourcesCount;
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

		Pointer<Buffer> CreateBuffer(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC* ResourceDesc, D3D12_RESOURCE_STATES InitialState, const char16_t* DebugName = nullptr);
		Pointer<Texture> CreateTexture(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC* ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* OptimizedClearValue = nullptr, const char16_t* DebugName = nullptr);

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

		Pointer<DescriptorHeap> RTDescriptorHeap, DSDescriptorHeap, CBSRUADescriptorHeap, SamplersDescriptorHeap;
		Pointer<DescriptorHeap> ConstantBufferDescriptorHeap, TexturesDescriptorHeap;

		Pointer<FrameDescriptorHeap> FrameResourcesDescriptorHeaps[2], FrameSamplersDescriptorHeaps[2];

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

		Pointer<Buffer> GPUCameraConstantBuffer, CPUCameraConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE CameraConstantBufferCBV;

		Pointer<Buffer> GPURenderTargetConstantBuffer, CPURenderTargetConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetConstantBufferCBV;

		COMRCPtr<ID3D12Heap> GPUMemory0, CPUMemory0;

		// ===============================================================================================================

		Pointer<Texture> GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferTexturesRTVs[2], GBufferTexturesSRVs[2];

		Pointer<Texture> DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureDSV, DepthBufferTextureSRV;

		Pointer<Buffer> GPUGBufferOpaquePassObjectsConstantBuffer, CPUGBufferOpaquePassObjectsConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferOpaquePassObjectsConstantBufferCBVs[20000];

		COMRCPtr<ID3D12Heap> GPUMemory1, CPUMemory1;

		// ===============================================================================================================

		Pointer<Texture> ResolvedDepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedDepthBufferTextureSRV;

		COMRCPtr<ID3D12Heap> GPUMemory2;

		// ===============================================================================================================

		Pointer<Texture> OcclusionBufferTexture;
		Pointer<Buffer> OcclusionBufferTextureReadback[2];
		D3D12_CPU_DESCRIPTOR_HANDLE OcclusionBufferTextureRTV;

		COMRCPtr<ID3D12PipelineState> OcclusionBufferPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory3, CPUMemory3;

		// ===============================================================================================================

		Pointer<Texture> CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapTexturesDSVs[4], CascadedShadowMapTexturesSRVs[4];

		Pointer<Buffer> GPUShadowMapCameraConstantBuffer, CPUShadowMapCameraConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMapCameraConstantBufferCBVs[4];

		Pointer<Buffer> GPUShadowMapPassObjectsConstantBuffers[4], CPUShadowMapPassObjectsConstantBuffers[4][2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMapPassObjectsConstantBufferCBVs[4][20000];

		COMRCPtr<ID3D12Heap> GPUMemory4, CPUMemory4;

		// ===============================================================================================================

		Pointer<Texture> ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskTextureRTV, ShadowMaskTextureSRV;

		Pointer<Buffer> GPUShadowResolveConstantBuffer, CPUShadowResolveConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowResolveConstantBufferCBV;

		COMRCPtr<ID3D12PipelineState> ShadowResolvePipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory5, CPUMemory5;

		// ===============================================================================================================

		Pointer<Texture> HDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureRTV, HDRSceneColorTextureSRV;

		Pointer<Buffer> GPULightingConstantBuffer, CPULightingConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightingConstantBufferCBV;

		Pointer<Buffer> GPUClusteredShadingConstantBuffer, CPUClusteredShadingConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ClusteredShadingConstantBufferCBV;

		Pointer<Buffer> GPULightClustersBuffer, CPULightClustersBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightClustersBufferSRV;

		Pointer<Buffer> GPULightIndicesBuffer, CPULightIndicesBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE LightIndicesBufferSRV;

		Pointer<Buffer> GPUPointLightsBuffer, CPUPointLightsBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE PointLightsBufferSRV;

		COMRCPtr<ID3D12PipelineState> DeferredLightingPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory6, CPUMemory6;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> SkyVertexBuffer, SkyIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SkyVertexBufferAddress, SkyIndexBufferAddress;
		Pointer<Buffer> GPUSkyConstantBuffer, CPUSkyConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE SkyConstantBufferCBV;
		COMRCPtr<ID3D12PipelineState> SkyPipelineState;
		COMRCPtr<ID3D12Resource> SkyTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SkyTextureSRV;

		COMRCPtr<ID3D12Resource> SunVertexBuffer, SunIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SunVertexBufferAddress, SunIndexBufferAddress;
		Pointer<Buffer> GPUSunConstantBuffer, CPUSunConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE SunConstantBufferCBV;
		COMRCPtr<ID3D12PipelineState> SunPipelineState;
		COMRCPtr<ID3D12Resource> SunTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SunTextureSRV;

		COMRCPtr<ID3D12PipelineState> FogPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory7, CPUMemory7;

		// ===============================================================================================================

		Pointer<Texture> ResolvedHDRSceneColorTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ResolvedHDRSceneColorTextureSRV;

		COMRCPtr<ID3D12Heap> GPUMemory8;

		// ===============================================================================================================

		Pointer<Texture> SceneLuminanceTextures[12];
		D3D12_CPU_DESCRIPTOR_HANDLE SceneLuminanceTexturesRTVs[12], SceneLuminanceTexturesSRVs[12];

		Pointer<Texture> AverageLuminanceTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE AverageLuminanceTextureRTV, AverageLuminanceTextureSRV;

		COMRCPtr<ID3D12PipelineState> LuminanceCalcPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceSumPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceAvgPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory9;

		// ===============================================================================================================

		Pointer<Texture> BloomTextures[3][7];
		D3D12_CPU_DESCRIPTOR_HANDLE BloomTexturesRTVs[3][7], BloomTexturesSRVs[3][7];

		COMRCPtr<ID3D12PipelineState> BrightPassPipelineState;
		COMRCPtr<ID3D12PipelineState> DownSamplePipelineState;
		COMRCPtr<ID3D12PipelineState> HorizontalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> VerticalBlurPipelineState;
		COMRCPtr<ID3D12PipelineState> UpSampleWithAddBlendPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory10;

		// ===============================================================================================================

		Pointer<Texture> ToneMappedImageTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ToneMappedImageTextureRTV;

		COMRCPtr<ID3D12PipelineState> HDRToneMappingPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory11;

		// ===============================================================================================================

		Pointer<Texture> DebugOcclusionBufferTexture;
		Pointer<Buffer> DebugOcclusionBufferTextureUpload[2];
		D3D12_CPU_DESCRIPTOR_HANDLE DebugOcclusionBufferTextureSRV;

		COMRCPtr<ID3D12PipelineState> DebugDrawOcclusionBufferPipelineState;

		COMRCPtr<ID3D12Heap> GPUMemory12, CPUMemory12;

		// ===============================================================================================================

		COMRCPtr<ID3D12Resource> BoundingBoxIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS BoundingBoxIndexBufferAddress;

		COMRCPtr<ID3D12PipelineState> DebugDrawBoundingBoxPipelineState;

		Pointer<Buffer> GPUConstantBuffer3, CPUConstantBuffers3[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs3[20000];

		COMRCPtr<ID3D12Heap> GPUMemory13, CPUMemory13;

		// ===============================================================================================================

		DynamicArray<RenderMesh*> RenderMeshDestructionQueue;
		DynamicArray<RenderMaterial*> RenderMaterialDestructionQueue;
		DynamicArray<RenderTexture*> RenderTextureDestructionQueue;

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		static const UINT MAX_MIP_LEVELS_IN_TEXTURE = 16;

		static const UINT MAX_PENDING_BARRIERS = 1000;

		D3D12_RESOURCE_BARRIER PendingResourceBarriers[MAX_PENDING_BARRIERS];
		UINT PendingResourceBarriersCount = 0;

		void ApplyPendingBarriers();
		void SwitchResourceState(ID3D12Resource* Resource, UINT SubResourceIndex, D3D12_RESOURCE_STATES OldState, D3D12_RESOURCE_STATES NewState);
		void SetBufferState(Pointer<Buffer>& BufferPtr, D3D12_RESOURCE_STATES NewState);
		void SetTextureState(Pointer<Texture>& TexturePtr, UINT SubResourceIndex, D3D12_RESOURCE_STATES NewState);

	#if WITH_EDITOR
		UINT EditorViewportWidth;
		UINT EditorViewportHeight;
	#endif

		inline SIZE_T GetOffsetForResource(D3D12_RESOURCE_DESC& ResourceDesc, D3D12_HEAP_DESC& HeapDesc);

		static const UINT VERTEX_SHADER_CONSTANT_BUFFERS = 0;
		static const UINT VERTEX_SHADER_SHADER_RESOURCES = 1;
		static const UINT VERTEX_SHADER_SAMPLERS = 2;
		static const UINT PIXEL_SHADER_CONSTANT_BUFFERS = 3;
		static const UINT PIXEL_SHADER_SHADER_RESOURCES = 4;
		static const UINT PIXEL_SHADER_SAMPLERS = 5;

		static const UINT COMPUTE_SHADER_CONSTANT_BUFFERS = 0;
		static const UINT COMPUTE_SHADER_SHADER_RESOURCES = 1;
		static const UINT COMPUTE_SHADER_SAMPLERS = 2;
		static const UINT COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS = 3;
};