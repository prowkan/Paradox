#pragma once

struct RenderMesh
{
	ID3D12Resource *VertexBuffer, *IndexBuffer;
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

class RenderSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

		RenderMesh* CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo);
		RenderTexture* CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo);
		RenderMaterial* CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo);

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
};