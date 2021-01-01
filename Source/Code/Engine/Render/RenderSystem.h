#pragma once

class RenderSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

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

		ID3D12RootSignature *RootSignature;

		ID3D12Resource *BackBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRTVs[2];

		ID3D12Resource *DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferDSV;

		ID3D12Resource *GPUConstantBuffer, *CPUConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE ConstantBufferCBVs[20000];

		D3D12_CPU_DESCRIPTOR_HANDLE Sampler;

		ID3D12Resource *VertexBuffers[4000], *IndexBuffers[4000];

		ID3D12PipelineState *PipelineStates[4000];

		ID3D12Resource *Textures[4000];
		D3D12_CPU_DESCRIPTOR_HANDLE TextureSRVs[4000];

		struct
		{
			XMFLOAT3 Location;
			XMFLOAT3 Rotation;
			XMFLOAT3 Scale;
			ID3D12Resource *VertexBuffer, *IndexBuffer;
			ID3D12PipelineState *PipelineState;
			D3D12_CPU_DESCRIPTOR_HANDLE TextureSRV;
		} RenderObjects[20000];
};