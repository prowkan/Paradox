#pragma once

#include <Containers/COMRCPtr.h>

#include "CullingSubSystem.h"
#include "ClusterizationSubSystem.h"

struct RenderMesh
{
	COMRCPtr<ID3D12Resource> VertexBuffer, IndexBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS VertexBufferAddress, IndexBufferAddress;
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
	void *VertexData;
	void *IndexData;
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

struct CompressedTexelBlockBC1
{
	uint16_t Colors[2];
	uint8_t Texels[4];
};

struct CompressedTexelBlockBC5
{
	uint8_t Red[2];
	uint8_t RedIndices[6];
	uint8_t Green[2];
	uint8_t GreenIndices[6];
};

struct Color
{
	float R, G, B;
};

inline Color operator+(const Color& Color1, const Color& Color2)
{
	Color Result;

	Result.R = Color1.R + Color2.R;
	Result.G = Color1.G + Color2.G;
	Result.B = Color1.B + Color2.B;

	return Result;
}

inline Color operator*(const float Scalar, const Color& color)
{
	Color Result;

	Result.R = Scalar * color.R;
	Result.G = Scalar * color.G;
	Result.B = Scalar * color.B;

	return Result;
}

inline Color operator*(const Color& color, const float Scalar)
{
	return operator*(Scalar, color);
}

inline Color operator/(const Color& color, const float Scalar)
{
	Color Result;

	Result.R = color.R / Scalar;
	Result.G = color.G / Scalar;
	Result.B = color.B / Scalar;

	return Result;
}

inline float DistanceBetweenColor(const Color& Color1, const Color& Color2)
{
	return powf(Color1.R - Color2.R, 2.0f) + powf(Color1.G - Color2.G, 2.0f) + powf(Color1.B - Color2.B, 2.0f);
}

class RenderPass;

#define SAFE_DX(Func) CheckDXCallResult(Func, u#Func);
#define UUIDOF(Value) __uuidof(Value), (void**)&Value

class DescriptorHeap
{
	public:

		DescriptorHeap()
		{
			DXDescriptorHeap = nullptr;
		}

		DescriptorHeap(ID3D12Device *DXDevice, const D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType, const UINT DescriptorsCount);

		D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor()
		{
			D3D12_CPU_DESCRIPTOR_HANDLE Descriptor;
			Descriptor.ptr = FirstDescriptor + AllocatedDescriptorsCount * DescriptorSize;
			++AllocatedDescriptorsCount;
			return Descriptor;
		}

	private:

		COMRCPtr<ID3D12DescriptorHeap> DXDescriptorHeap;
		SIZE_T FirstDescriptor;
		UINT DescriptorSize;

		UINT AllocatedDescriptorsCount;
};

class RootSignature
{
	public:

		RootSignature()
		{
			DXRootSignature = nullptr;
		}

		RootSignature(ID3D12Device *DXDevice, const D3D12_ROOT_SIGNATURE_DESC& RootSignatureDesc);

		~RootSignature();

		operator ID3D12RootSignature*()
		{
			return DXRootSignature;
		}

		const D3D12_ROOT_SIGNATURE_DESC& GetRootSignatureDesc() { return DXRootSignatureDesc; };

	private:

		COMRCPtr<ID3D12RootSignature> DXRootSignature;

		D3D12_ROOT_SIGNATURE_DESC DXRootSignatureDesc;
};

class FrameDescriptorHeap;

class DescriptorTable
{
	public:

		DescriptorTable() : CurrentFrameIndex(nullptr)
		{
			DescriptorsArray = nullptr;
			ArrayOfOnes = nullptr;
			DescriptorsCountInTable = 0;
		}

		DescriptorTable(const DescriptorTable& OtherDescriptorTable) : CurrentFrameIndex(OtherDescriptorTable.CurrentFrameIndex)
		{
			Device = OtherDescriptorTable.Device;

			DescriptorHeapType = OtherDescriptorTable.DescriptorHeapType;

			DescriptorsCountInTable = OtherDescriptorTable.DescriptorsCountInTable;
			OffsetInDescriptorHeap = OtherDescriptorTable.OffsetInDescriptorHeap;

			this->FirstDescriptorsInTableCPU[0] = OtherDescriptorTable.FirstDescriptorsInTableCPU[0];
			this->FirstDescriptorsInTableCPU[1] = OtherDescriptorTable.FirstDescriptorsInTableCPU[1];
			this->FirstDescriptorsInTableGPU[0] = OtherDescriptorTable.FirstDescriptorsInTableGPU[0];
			this->FirstDescriptorsInTableGPU[1] = OtherDescriptorTable.FirstDescriptorsInTableGPU[1];

			DescriptorsCountInTable = OtherDescriptorTable.DescriptorsCountInTable;
			DescriptorsArray = new D3D12_CPU_DESCRIPTOR_HANDLE[OtherDescriptorTable.DescriptorsCountInTable];
			ArrayOfOnes = new UINT[OtherDescriptorTable.DescriptorsCountInTable];

			for (UINT i = 0; i < DescriptorsCountInTable; i++) ArrayOfOnes[i] = 1;
		}

		DescriptorTable(
			const UINT DescriptorsCountInTable,
			const UINT OffsetInDescriptorHeap,
			const SIZE_T FirstDescriptorsInTableCPU0,
			const SIZE_T FirstDescriptorsInTableCPU1,
			const UINT64 FirstDescriptorsInTableGPU0,
			const UINT64 FirstDescriptorsInTableGPU1,
			D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType,
			ID3D12Device* DXDevice,
			UINT* FrameIndexRef
		) : 
			DescriptorsCountInTable(DescriptorsCountInTable), 
			OffsetInDescriptorHeap(OffsetInDescriptorHeap),
			DescriptorHeapType(DescriptorHeapType),
			CurrentFrameIndex(FrameIndexRef)
		{
			DescriptorsArray = new D3D12_CPU_DESCRIPTOR_HANDLE[DescriptorsCountInTable];
			ArrayOfOnes = new UINT[DescriptorsCountInTable];

			for (UINT i = 0; i < DescriptorsCountInTable; i++) ArrayOfOnes[i] = 1;

			this->FirstDescriptorsInTableCPU[0].ptr = FirstDescriptorsInTableCPU0;
			this->FirstDescriptorsInTableCPU[1].ptr = FirstDescriptorsInTableCPU1;
			this->FirstDescriptorsInTableGPU[0].ptr = FirstDescriptorsInTableGPU0;
			this->FirstDescriptorsInTableGPU[1].ptr = FirstDescriptorsInTableGPU1;

			Device = DXDevice;
		}

		~DescriptorTable()
		{
			delete[] DescriptorsArray;
			delete[] ArrayOfOnes;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE& operator[](size_t Index)
		{
			return DescriptorsArray[Index];
		}

		void UpdateDescriptorTable()
		{
			Device->CopyDescriptors(1, &FirstDescriptorsInTableCPU[*CurrentFrameIndex], &DescriptorsCountInTable, DescriptorsCountInTable, DescriptorsArray, ArrayOfOnes, DescriptorHeapType);
		}

		void SetTableSize(const UINT NewTableSize)
		{
			DescriptorsCountInTable = NewTableSize;
		}

		operator D3D12_GPU_DESCRIPTOR_HANDLE()
		{
			return FirstDescriptorsInTableGPU[*CurrentFrameIndex];
		}

		DescriptorTable& operator=(const DescriptorTable& OtherDescriptorTable)
		{
			Device = OtherDescriptorTable.Device;

			CurrentFrameIndex = OtherDescriptorTable.CurrentFrameIndex;

			DescriptorHeapType = OtherDescriptorTable.DescriptorHeapType;

			DescriptorsCountInTable = OtherDescriptorTable.DescriptorsCountInTable;
			OffsetInDescriptorHeap = OtherDescriptorTable.OffsetInDescriptorHeap;

			this->FirstDescriptorsInTableCPU[0] = OtherDescriptorTable.FirstDescriptorsInTableCPU[0];
			this->FirstDescriptorsInTableCPU[1] = OtherDescriptorTable.FirstDescriptorsInTableCPU[1];
			this->FirstDescriptorsInTableGPU[0] = OtherDescriptorTable.FirstDescriptorsInTableGPU[0];
			this->FirstDescriptorsInTableGPU[1] = OtherDescriptorTable.FirstDescriptorsInTableGPU[1];

			DescriptorsCountInTable = OtherDescriptorTable.DescriptorsCountInTable;
			delete[] DescriptorsArray;
			delete[] ArrayOfOnes;
			DescriptorsArray = new D3D12_CPU_DESCRIPTOR_HANDLE[OtherDescriptorTable.DescriptorsCountInTable];
			ArrayOfOnes = new UINT[OtherDescriptorTable.DescriptorsCountInTable];

			for (UINT i = 0; i < DescriptorsCountInTable; i++) ArrayOfOnes[i] = 1;
			return *this;
		}

	private:

		D3D12_CPU_DESCRIPTOR_HANDLE *DescriptorsArray;

		D3D12_CPU_DESCRIPTOR_HANDLE FirstDescriptorsInTableCPU[2];
		D3D12_GPU_DESCRIPTOR_HANDLE FirstDescriptorsInTableGPU[2];

		D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType;

		UINT DescriptorsCountInTable, OffsetInDescriptorHeap, *ArrayOfOnes;
		UINT *CurrentFrameIndex;

		ID3D12Device *Device;
};

class FrameDescriptorHeap
{
	public:

		FrameDescriptorHeap() : CurrentFrameIndex(nullptr)
		{
			DXDescriptorHeaps[0] = nullptr;
			DXDescriptorHeaps[1] = nullptr;
		}

		FrameDescriptorHeap(ID3D12Device *DXDevice, const D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType, const UINT DescriptorsCount, UINT* FrameIndexRef);

		DescriptorTable AllocateDescriptorTable(const D3D12_ROOT_PARAMETER& RootParameter);

		ID3D12DescriptorHeap* GetDXDescriptorHeap() { return DXDescriptorHeaps[*CurrentFrameIndex]; }

	private:

		COMRCPtr<ID3D12DescriptorHeap> DXDescriptorHeaps[2];

		D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType;

		SIZE_T FirstDescriptorsCPU[2];
		UINT64 FirstDescriptorsGPU[2];

		UINT DescriptorSize = 0;

		UINT AllocatedDescriptorsForTables = 0;

		ID3D12Device *Device;

		UINT *CurrentFrameIndex;
};

struct Buffer
{
	COMRCPtr<ID3D12Resource> DXBuffer;
	D3D12_RESOURCE_STATES DXBufferState;
};

struct Texture
{
	COMRCPtr<ID3D12Resource> DXTexture;
	D3D12_RESOURCE_STATES *DXTextureSubResourceStates;
	UINT SubResourcesCount;
};

class DX12Helpers
{
	public:

		static inline D3D12_HEAP_PROPERTIES CreateDXHeapProperties(D3D12_HEAP_TYPE HeapType)
		{
			D3D12_HEAP_PROPERTIES HeapProperties;

			HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			HeapProperties.CreationNodeMask = 0;
			HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
			HeapProperties.Type = HeapType;
			HeapProperties.VisibleNodeMask = 0;

			return HeapProperties;
		}

		static inline D3D12_RESOURCE_DESC CreateDXResourceDescBuffer(UINT64 BufferSize, D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE)
		{
			D3D12_RESOURCE_DESC ResourceDesc;
			ResourceDesc.Alignment = 0;
			ResourceDesc.DepthOrArraySize = 1;
			ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
			ResourceDesc.Flags = ResourceFlags;
			ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			ResourceDesc.Height = 1;
			ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			ResourceDesc.MipLevels = 1;
			ResourceDesc.SampleDesc.Count = 1;
			ResourceDesc.SampleDesc.Quality = 0;
			ResourceDesc.Width = BufferSize;

			return ResourceDesc;
		}

		static inline D3D12_RESOURCE_DESC CreateDXResourceDescTexture2D(UINT64 Width, UINT Height, DXGI_FORMAT Format, D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE, UINT MIPLevels = 1, UINT SamplesCount = 1)
		{
			D3D12_RESOURCE_DESC ResourceDesc;
			ResourceDesc.Alignment = 0;
			ResourceDesc.DepthOrArraySize = 1;
			ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			ResourceDesc.Flags = ResourceFlags;
			ResourceDesc.Format = Format;
			ResourceDesc.Height = Height;
			ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
			ResourceDesc.MipLevels = MIPLevels;
			ResourceDesc.SampleDesc.Count = SamplesCount;
			ResourceDesc.SampleDesc.Quality = 0;
			ResourceDesc.Width = Width;

			return ResourceDesc;
		}
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

		Buffer CreateBuffer(const D3D12_HEAP_PROPERTIES& HeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue);
		Buffer CreateBuffer(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue);
		Texture CreateTexture(const D3D12_HEAP_PROPERTIES& HeapProperties, D3D12_HEAP_FLAGS HeapFlags, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue);
		Texture CreateTexture(ID3D12Heap* Heap, UINT64 HeapOffset, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue);

		CullingSubSystem& GetCullingSubSystem() { return cullingSubSystem; }
		ClusterizationSubSystem& GetClusterizationSubSystem() { return clusterizationSubSystem; }

		ID3D12Device* GetDevice() { return Device; }
		ID3D12GraphicsCommandList*& GetCommandList() { return CommandList.Pointer; }

		int GetResolutionWidth() { return ResolutionWidth; }
		int GetResolutionHeight() { return ResolutionHeight; }

		Texture* GetCurrentBackBufferTexture() { return &BackBufferTextures[CurrentFrameIndex]; }

		DescriptorHeap& GetRTDescriptorHeap() { return RTDescriptorHeap; }
		DescriptorHeap& GetDSDescriptorHeap() { return DSDescriptorHeap; }
		DescriptorHeap& GetCBSRUADescriptorHeap() { return CBSRUADescriptorHeap; }
		DescriptorHeap& GetSamplersDescriptorHeap() { return SamplersDescriptorHeap; }

		DescriptorHeap& GetConstantBufferDescriptorHeap() { return ConstantBufferDescriptorHeap; }
		DescriptorHeap& GetTexturesDescriptorHeap() { return TexturesDescriptorHeap; }

		RootSignature& GetGraphicsRootSignature() { return GraphicsRootSignature; }
		RootSignature& GetComputeRootSignature() { return ComputeRootSignature; }

		UINT GetCurrentFrameIndex() { return CurrentFrameIndex; }

		D3D12_SHADER_BYTECODE GetFullScreenQuadVertexShader() { return FullScreenQuadVertexShader; }

		FrameDescriptorHeap& GetFrameResourcesDescriptorHeap() { return FrameResourcesDescriptorHeap; }
		FrameDescriptorHeap& GetFrameSamplersDescriptorHeap() { return FrameSamplersDescriptorHeap; }

		DescriptorTable& GetTextureSamplerTable(){ return TextureSamplerTable; }
		DescriptorTable& GetShadowMapSamplerTable() { return ShadowMapSamplerTable; }
		DescriptorTable& GetBiLinearSamplerTable() { return BiLinearSamplerTable; }
		DescriptorTable& GetMinSamplerTable() { return MinSamplerTable; }

		ID3D12Resource* GetUploadBuffer() { return UploadBuffer; }

		ID3D12CommandQueue* GetCommandQueue() { return CommandQueue; }

		ID3D12Fence* GetCopySyncFence() { return CopySyncFence; }
		HANDLE GetCopySyncEvent() { return CopySyncEvent; }

		ID3D12CommandAllocator* GetCommandAllocator(UINT Index) { return CommandAllocators[Index]; }

		template<typename T>
		T* GetRenderPass()
		{
			for (RenderPass* renderPass : RenderPasses)
			{
				if (dynamic_cast<T*>(renderPass))
				{
					return (T*)renderPass;
				}
			}

			return nullptr;
		}

	private:

		D3D12_SHADER_BYTECODE FullScreenQuadVertexShader;

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

		DescriptorHeap RTDescriptorHeap, DSDescriptorHeap, CBSRUADescriptorHeap, SamplersDescriptorHeap;
		DescriptorHeap ConstantBufferDescriptorHeap, TexturesDescriptorHeap;

		FrameDescriptorHeap FrameResourcesDescriptorHeap, FrameSamplersDescriptorHeap;

		RootSignature GraphicsRootSignature, ComputeRootSignature;

		Texture BackBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferTexturesRTVs[2];

		// ===============================================================================================================

		D3D12_CPU_DESCRIPTOR_HANDLE TextureSampler, ShadowMapSampler, BiLinearSampler, MinSampler;

		DescriptorTable TextureSamplerTable, ShadowMapSamplerTable, BiLinearSamplerTable, MinSamplerTable;

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

		vector<RenderMesh*> RenderMeshDestructionQueue;
		vector<RenderMaterial*> RenderMaterialDestructionQueue;
		vector<RenderTexture*> RenderTextureDestructionQueue;

		CullingSubSystem cullingSubSystem;
		ClusterizationSubSystem clusterizationSubSystem;

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		static const UINT MAX_MIP_LEVELS_IN_TEXTURE = 16;

	public:
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

	private:
		static const UINT MAX_PENDING_BARRIERS_COUNT = 200;
		UINT PendingBarriersCount = 0;

		D3D12_RESOURCE_BARRIER PendingResourceBarriers[MAX_PENDING_BARRIERS_COUNT];

		//void SwitchResourceState(ID3D12Resource *Resource, const UINT SubResource, const D3D12_RESOURCE_STATES OldState, const D3D12_RESOURCE_STATES NewState);
	public:
		void SwitchResourceState(Buffer& buffer, const D3D12_RESOURCE_STATES NewState);
		void SwitchResourceState(Texture& texture, const UINT SubResource, const D3D12_RESOURCE_STATES NewState);
		void ApplyPendingBarriers();

	private: 
		vector<RenderPass*> RenderPasses;
};