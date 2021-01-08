#pragma once

#include "CullingSubSystem.h"

struct RenderMesh
{
	ID3D12Resource *VertexBuffer, *IndexBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS VertexBufferAddress, IndexBufferAddress;
};

struct RenderTexture
{
	ID3D12Resource *Texture;
	D3D12_CPU_DESCRIPTOR_HANDLE TextureSRV;
};

struct RenderMaterial
{
	ID3D12PipelineState *PipelineState;
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

#define SAFE_DX(Func) CheckDXCallResult(Func, L#Func);
#define SAFE_RELEASE(Object) if (Object) { ULONG RefCount = Object->Release(); Object = nullptr; }

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

		ID3D12Device *Device;
		IDXGISwapChain4 *SwapChain;

		int ResolutionWidth;
		int ResolutionHeight;

		ID3D12CommandQueue *CommandQueue;
		ID3D12CommandAllocator *CommandAllocators[2];
		ID3D12GraphicsCommandList *CommandList;

		UINT CurrentBackBufferIndex, CurrentFrameIndex;

		ID3D12Fence *Fences[2];
		HANDLE Event;

		ID3D12DescriptorHeap *RTDescriptorHeap, *DSDescriptorHeap, *CBSRUADescriptorHeap, *SamplersDescriptorHeap;
		ID3D12DescriptorHeap *ConstantBufferDescriptorHeap, *TexturesDescriptorHeap;
		ID3D12DescriptorHeap *FrameResourcesDescriptorHeaps[2], *FrameSamplersDescriptorHeaps[2];

		UINT TexturesDescriptorsCount = 0;

		ID3D12RootSignature *RootSignature;

		ID3D12Resource *BackBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRTVs[2];

		ID3D12Resource *DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferDSV;

		ID3D12Resource *GPUConstantBuffer, *CPUConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[20000];

		D3D12_CPU_DESCRIPTOR_HANDLE Sampler;

		static const UINT MAX_MEMORY_HEAPS_COUNT = 200;
		static const SIZE_T BUFFER_MEMORY_HEAP_SIZE = 16 * 1024 * 1024, TEXTURE_MEMORY_HEAP_SIZE = 256 * 1024 * 1024;
		static const SIZE_T UPLOAD_HEAP_SIZE = 64 * 1024 * 1024;

		ID3D12Heap *BufferMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { nullptr }, *TextureMemoryHeaps[MAX_MEMORY_HEAPS_COUNT] = { nullptr };
		size_t BufferMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 }, TextureMemoryHeapOffsets[MAX_MEMORY_HEAPS_COUNT] = { 0 };
		int CurrentBufferMemoryHeapIndex = 0, CurrentTextureMemoryHeapIndex = 0;

		ID3D12Heap *UploadHeap;
		ID3D12Resource *UploadBuffer;
		size_t UploadBufferOffset = 0;

		vector<RenderMesh*> RenderMeshDestructionQueue;
		vector<RenderMaterial*> RenderMaterialDestructionQueue;
		vector<RenderTexture*> RenderTextureDestructionQueue;

		CullingSubSystem cullingSubSystem;

		inline void CheckDXCallResult(HRESULT hr, const wchar_t* Function);
		inline const wchar_t* GetDXErrorMessageFromHRESULT(HRESULT hr);
};