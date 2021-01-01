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

		ID3D12CommandQueue *CommandQueue;
		ID3D12CommandAllocator *CommandAllocators[2];
		ID3D12GraphicsCommandList *CommandList;

		ID3D12DescriptorHeap *RTDescriptorHeap;

		ID3D12Resource *BackBufferTextures[2];
		D3D12_CPU_DESCRIPTOR_HANDLE BackBufferRTVs[2];

		UINT CurrentBackBufferIndex, CurrentFrameIndex;

		ID3D12Fence *Fences[2];
		HANDLE Event;
};