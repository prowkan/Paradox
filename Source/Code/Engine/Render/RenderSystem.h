#pragma once

#include <Containers/COMRCPtr.h>

#include "CullingSubSystem.h"

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

	private:

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
		COMRCPtr<ID3D12DescriptorHeap> FrameResourcesDescriptorHeaps[2], FrameSamplersDescriptorHeaps[2];

		UINT RTDescriptorsCount = 0, DSDescriptorsCount = 0, CBSRUADescriptorsCount = 0, SamplersDescriptorsCount = 0;
		UINT ConstantBufferDescriptorsCount = 0, TexturesDescriptorsCount = 0;

		COMRCPtr<ID3D12RootSignature> GraphisRootSignature, ComputeRootSignature;

		COMRCPtr<ID3D12Resource> BackBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRTVs[2];

		COMRCPtr<ID3D12Resource> GBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE GBufferRTVs[2], GBufferSRVs[2];

		COMRCPtr<ID3D12Resource> DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferDSV, DepthBufferSRV;

		COMRCPtr<ID3D12Resource> CascadedShadowMapTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE CascadedShadowMapDSVs[4], CascadedShadowMapSRVs[4];

		COMRCPtr<ID3D12Resource> ShadowMaskTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowMaskRTV, ShadowMaskSRV;

		COMRCPtr<ID3D12Resource> LBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE LBufferRTV, LBufferSRV;

		COMRCPtr<ID3D12Resource> SceneLuminanceTextures[4];
		D3D12_CPU_DESCRIPTOR_HANDLE SceneLuminanceUAVs[4], SceneLuminanceSRVs[4];

		COMRCPtr<ID3D12Resource> AverageLuminanceTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE AverageLuminanceUAV, AverageLuminanceSRV;

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

		COMRCPtr<ID3D12Resource> GPUShadowResolveConstantBuffer, CPUShadowResolveConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ShadowResolveConstantBufferCBV;

		COMRCPtr<ID3D12Resource> GPUDeferredLightingConstantBuffer, CPUDeferredLightingConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE DeferredLightingConstantBufferCBV;

		COMRCPtr<ID3D12PipelineState> ShadowResolvePipelineState;
		COMRCPtr<ID3D12PipelineState> DeferredLightingPipelineState;
		COMRCPtr<ID3D12PipelineState> FogPipelineState;
		COMRCPtr<ID3D12PipelineState> HDRToneMappingPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceCalcPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceSumPipelineState;
		COMRCPtr<ID3D12PipelineState> LuminanceAvgPipelineState;

		COMRCPtr<ID3D12Resource> GPUConstantBuffer, CPUConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[20000];

		COMRCPtr<ID3D12Resource> GPUConstantBuffers2[4], CPUConstantBuffers2[4][2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs2[4][20000];

		D3D12_CPU_DESCRIPTOR_HANDLE TextureSampler, ShadowMapSampler;

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

		inline void CheckDXCallResult(HRESULT hr, const char16_t* Function);
		inline const char16_t* GetDXErrorMessageFromHRESULT(HRESULT hr);

		static const UINT MAX_MIP_LEVELS_IN_TEXTURE = 16;
};