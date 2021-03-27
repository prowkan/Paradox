// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "OcclusionBufferPass.h"

#include <Engine/Engine.h>

void OcclusionBufferPass::Init(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ResourceDesc.Height = 144;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 256;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE, &ClearValue, UUIDOF(OcclusionBufferTexture)));

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	OcclusionBufferTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	RTDescriptorsCount++;

	Device->CreateRenderTargetView(OcclusionBufferTexture, &RTVDesc, OcclusionBufferTextureRTV);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

	UINT NumRows;
	UINT64 RowSizeInBytes, TotalBytes;

	Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = TotalBytes;

	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(OcclusionBufferTextureReadback[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(OcclusionBufferTextureReadback[1])));

	HANDLE OcclusionBufferPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/OcclusionBuffer.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER OcclusionBufferPixelShaderByteCodeLength;
	Result = GetFileSizeEx(OcclusionBufferPixelShaderFile, &OcclusionBufferPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> OcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(OcclusionBufferPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(OcclusionBufferPixelShaderFile, OcclusionBufferPixelShaderByteCodeData, (DWORD)OcclusionBufferPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(OcclusionBufferPixelShaderFile);

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
	GraphicsPipelineStateDesc.PS.BytecodeLength = OcclusionBufferPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = OcclusionBufferPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(OcclusionBufferPipelineState)));
}

void OcclusionBufferPass::Execute(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_BARRIER ResourceBarriers[2];

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ResolvedDepthBufferTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = OcclusionBufferTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &OcclusionBufferTextureRTV, TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = 144.0f;
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = 256.0f;

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = 144;
	ScissorRect.left = 0;
	ScissorRect.right = 256;
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderSystem.GetCommandList()->DiscardResource(OcclusionBufferTexture, nullptr);

	Device->CopyDescriptorsSimple(1, SamplerCPUHandle, MinSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SamplerCPUHandle.ptr += SamplerHandleSize;

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(5, D3D12_GPU_DESCRIPTOR_HANDLE{ SamplerGPUHandle.ptr + 0 * ResourceHandleSize });
	SamplerGPUHandle.ptr += SamplerHandleSize;

	UINT DestRangeSize = 1;
	UINT SourceRangeSizes[1] = { 1 };
	D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[1] = { ResolvedDepthBufferTextureSRV };

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetPipelineState(OcclusionBufferPipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

	ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = OcclusionBufferTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(1, ResourceBarriers);

	D3D12_RESOURCE_DESC ResourceDesc = OcclusionBufferTexture->GetDesc();

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

	UINT NumRows;
	UINT64 RowSizeInBytes, TotalBytes;

	Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

	D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

	SourceTextureCopyLocation.pResource = OcclusionBufferTexture;
	SourceTextureCopyLocation.SubresourceIndex = 0;
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	DestTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
	DestTextureCopyLocation.pResource = OcclusionBufferTextureReadback[CurrentFrameIndex];
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	renderSystem.GetCommandList()->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

	float *OcclusionBufferData = cullingSubSystem.GetOcclusionBufferData();

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = TotalBytes;

	WrittenRange.Begin = 0;
	WrittenRange.End = 0;

	void *MappedData;

	OcclusionBufferTextureReadback[CurrentFrameIndex]->Map(0, &ReadRange, &MappedData);

	for (UINT i = 0; i < NumRows; i++)
	{
		memcpy((BYTE*)OcclusionBufferData + i * RowSizeInBytes, (BYTE*)MappedData + i * PlacedSubResourceFootPrint.Footprint.RowPitch, RowSizeInBytes);
	}

	OcclusionBufferTextureReadback[CurrentFrameIndex]->Unmap(0, &WrittenRange);
}