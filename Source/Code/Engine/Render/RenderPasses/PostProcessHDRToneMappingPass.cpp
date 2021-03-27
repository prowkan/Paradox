// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessHDRToneMappingPass.h"

#include <Engine/Engine.h>

void PostProcessHDRToneMappingPass::Init(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE, &ClearValue, UUIDOF(ToneMappedImageTexture)));

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	ToneMappedImageTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	RTDescriptorsCount++;

	Device->CreateRenderTargetView(ToneMappedImageTexture, &RTVDesc, ToneMappedImageTextureRTV);

	HANDLE HDRToneMappingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HDRToneMapping.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER HDRToneMappingPixelShaderByteCodeLength;
	Result = GetFileSizeEx(HDRToneMappingPixelShaderFile, &HDRToneMappingPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HDRToneMappingPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(HDRToneMappingPixelShaderFile, HDRToneMappingPixelShaderByteCodeData, (DWORD)HDRToneMappingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(HDRToneMappingPixelShaderFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = HDRToneMappingPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = HDRToneMappingPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(HDRToneMappingPipelineState)));
}

void PostProcessHDRToneMappingPass::Execute(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_BARRIER ResourceBarriers[2];

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = OutputBloomTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ToneMappedImageTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &ToneMappedImageTextureRTV, TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = FLOAT(ResolutionHeight);
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = FLOAT(ResolutionWidth);

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = ResolutionHeight;
	ScissorRect.left = 0;
	ScissorRect.right = ResolutionWidth;
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderSystem.GetCommandList()->DiscardResource(ToneMappedImageTexture, nullptr);

	UINT DestRangeSize = 2;
	UINT SourceRangeSizes[2] = { 1, 1 };
	D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { HDRSceneColorTextureSRV, OutputBloomTextureSRV };

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetPipelineState(HDRToneMappingPipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

	ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);
}