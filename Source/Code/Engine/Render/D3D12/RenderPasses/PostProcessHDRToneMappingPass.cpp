// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessHDRToneMappingPass.h"

#include "DeferredLightingPass.h"
#include "PostProcessBloomPass.h"

#include <Engine/Engine.h>

#undef SAFE_DX
#define SAFE_DX(Func) Func

void PostProcessHDRToneMappingPass::Init(RenderDeviceD3D12& renderDevice)
{
	HDRSceneColorTextureSRV = ((DeferredLightingPass*)renderDevice.GetRenderPass("DeferredLightingPass"))->GetHDRSceneColorTextureSRV();
	OutputBloomTexture = ((PostProcessBloomPass*)renderDevice.GetRenderPass("PostProcessBloomPass"))->GetOutputBloomTexture();
	OutputBloomTextureSRV = ((PostProcessBloomPass*)renderDevice.GetRenderPass("PostProcessBloomPass"))->GetOutputBloomTextureSRV();

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	ToneMappedImageTexture = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderDevice.GetResolutionWidth(), renderDevice.GetResolutionHeight(), DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 1, 8), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	ToneMappedImageTextureRTV = renderDevice.GetRTDescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateRenderTargetView(ToneMappedImageTexture.DXTexture, &RTVDesc, ToneMappedImageTextureRTV);

	SIZE_T HDRToneMappingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("HDRToneMapping");
	ScopedMemoryBlockArray<BYTE> HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HDRToneMappingPixelShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("HDRToneMapping", HDRToneMappingPixelShaderByteCodeData);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderDevice.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = HDRToneMappingPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = HDRToneMappingPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderDevice.GetFullScreenQuadVertexShader();

	SAFE_DX(renderDevice.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(HDRToneMappingPipelineState)));

	HDRToneMappingPassSRTable = renderDevice.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderDevice.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderDeviceD3D12::PIXEL_SHADER_SHADER_RESOURCES]);
}

void PostProcessHDRToneMappingPass::Execute(RenderDeviceD3D12& renderDevice)
{
	renderDevice.SwitchResourceState(*OutputBloomTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderDevice.SwitchResourceState(ToneMappedImageTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderDevice.GetCommandList()->OMSetRenderTargets(1, &ToneMappedImageTextureRTV, TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = FLOAT(renderDevice.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = FLOAT(renderDevice.GetResolutionWidth());

	renderDevice.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = renderDevice.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderDevice.GetResolutionWidth();
	ScissorRect.top = 0;

	renderDevice.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	HDRToneMappingPassSRTable[0] = HDRSceneColorTextureSRV;
	HDRToneMappingPassSRTable[1] = OutputBloomTextureSRV;
	HDRToneMappingPassSRTable.SetTableSize(2);
	HDRToneMappingPassSRTable.UpdateDescriptorTable();

	renderDevice.GetCommandList()->DiscardResource(ToneMappedImageTexture.DXTexture, nullptr);

	renderDevice.GetCommandList()->SetPipelineState(HDRToneMappingPipelineState);

	renderDevice.GetCommandList()->SetGraphicsRootDescriptorTable(RenderDeviceD3D12::PIXEL_SHADER_SHADER_RESOURCES, HDRToneMappingPassSRTable);

	renderDevice.GetCommandList()->DrawInstanced(4, 1, 0, 0);
}