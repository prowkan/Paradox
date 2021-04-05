#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class SkyAndFogPass : public RenderPass
{
	public:

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "SkyAndFogPass"; }

	private:

		Texture *DepthBufferTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferTextureSRV, DepthBufferTextureDSV;

		D3D12_CPU_DESCRIPTOR_HANDLE HDRSceneColorTextureRTV;

		COMRCPtr<ID3D12Resource> SkyVertexBuffer, SkyIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SkyVertexBufferAddress, SkyIndexBufferAddress;
		Buffer GPUSkyConstantBuffer, CPUSkyConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE SkyConstantBufferCBV;
		COMRCPtr<ID3D12PipelineState> SkyPipelineState;
		COMRCPtr<ID3D12Resource> SkyTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SkyTextureSRV;

		COMRCPtr<ID3D12Resource> SunVertexBuffer, SunIndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS SunVertexBufferAddress, SunIndexBufferAddress;
		Buffer GPUSunConstantBuffer, CPUSunConstantBuffers[2];
		D3D12_CPU_DESCRIPTOR_HANDLE SunConstantBufferCBV;
		COMRCPtr<ID3D12PipelineState> SunPipelineState;
		COMRCPtr<ID3D12Resource> SunTexture;
		D3D12_CPU_DESCRIPTOR_HANDLE SunTextureSRV;

		COMRCPtr<ID3D12PipelineState> FogPipelineState;

		DescriptorTable FogSRTable, SkyCBTable, SkySRTable, SunCBTable, SunSRTable;
};