// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "OcclusionBufferPass.h"

#include "MSAADepthBufferResolvePass.h"

#include <Engine/Engine.h>

#undef SAFE_DX
#define SAFE_DX(Func) Func

void OcclusionBufferPass::Init(RenderDeviceD3D12& renderDevice)
{
	ResolvedDepthBufferTexture = ((MSAADepthBufferResolvePass*)renderDevice.GetRenderPass("MSAADepthBufferResolvePass"))->GetResolvedDepthBufferTexture();
	ResolvedDepthBufferTextureSRV = ((MSAADepthBufferResolvePass*)renderDevice.GetRenderPass("MSAADepthBufferResolvePass"))->GetResolvedDepthBufferTextureSRV();

	D3D12_RESOURCE_DESC ResourceDesc = DX12Helpers::CreateDXResourceDescTexture2D(256, 144, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;

	OcclusionBufferTexture = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE, &ClearValue);

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	OcclusionBufferTextureRTV = renderDevice.GetRTDescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateRenderTargetView(OcclusionBufferTexture.DXTexture, &RTVDesc, OcclusionBufferTextureRTV);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

	UINT NumRows;
	UINT64 RowSizeInBytes, TotalBytes;

	renderDevice.GetDevice()->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

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

	OcclusionBufferTextureReadback[0] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr);
	OcclusionBufferTextureReadback[1] = renderDevice.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr);
	
	SIZE_T OcclusionBufferPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("ShaderModel51.OcclusionBuffer");
	ScopedMemoryBlockArray<BYTE> OcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(OcclusionBufferPixelShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("ShaderModel51.OcclusionBuffer", OcclusionBufferPixelShaderByteCodeData);

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
	GraphicsPipelineStateDesc.PS.BytecodeLength = OcclusionBufferPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = OcclusionBufferPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderDevice.GetFullScreenQuadVertexShader();

	SAFE_DX(renderDevice.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(OcclusionBufferPipelineState)));

	OcclusionBufferPassSRTable = renderDevice.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderDevice.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderDeviceD3D12::PIXEL_SHADER_SHADER_RESOURCES]);
}

void OcclusionBufferPass::Execute(RenderDeviceD3D12& renderDevice)
{
	OcclusionBufferPassSRTable[0] = ResolvedDepthBufferTextureSRV;
	OcclusionBufferPassSRTable.SetTableSize(1);
	OcclusionBufferPassSRTable.UpdateDescriptorTable();

	renderDevice.SwitchResourceState(*ResolvedDepthBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderDevice.SwitchResourceState(OcclusionBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);

	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderDevice.GetCommandList()->OMSetRenderTargets(1, &OcclusionBufferTextureRTV, TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = 144.0f;
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = 256.0f;

	renderDevice.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = 144;
	ScissorRect.left = 0;
	ScissorRect.right = 256;
	ScissorRect.top = 0;

	renderDevice.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderDevice.GetCommandList()->DiscardResource(OcclusionBufferTexture.DXTexture, nullptr);

	renderDevice.GetCommandList()->SetGraphicsRootDescriptorTable(RenderDeviceD3D12::PIXEL_SHADER_SAMPLERS, renderDevice.GetMinSamplerTable());

	renderDevice.GetCommandList()->SetPipelineState(OcclusionBufferPipelineState);

	renderDevice.GetCommandList()->SetGraphicsRootDescriptorTable(RenderDeviceD3D12::PIXEL_SHADER_SHADER_RESOURCES, OcclusionBufferPassSRTable);

	renderDevice.GetCommandList()->DrawInstanced(4, 1, 0, 0);

	renderDevice.SwitchResourceState(OcclusionBufferTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE);

	renderDevice.ApplyPendingBarriers();

	D3D12_RESOURCE_DESC ResourceDesc = OcclusionBufferTexture.DXTexture->GetDesc();

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

	UINT NumRows;
	UINT64 RowSizeInBytes, TotalBytes;

	renderDevice.GetDevice()->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

	D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

	SourceTextureCopyLocation.pResource = OcclusionBufferTexture.DXTexture;
	SourceTextureCopyLocation.SubresourceIndex = 0;
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	DestTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
	DestTextureCopyLocation.pResource = OcclusionBufferTextureReadback[renderDevice.GetCurrentFrameIndex()].DXBuffer;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	renderDevice.GetCommandList()->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

	float *OcclusionBufferData = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetOcclusionBufferData();

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = TotalBytes;

	WrittenRange.Begin = 0;
	WrittenRange.End = 0;

	void *MappedData;

	SAFE_DX(OcclusionBufferTextureReadback[renderDevice.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &MappedData));

	for (UINT i = 0; i < NumRows; i++)
	{
		memcpy((BYTE*)OcclusionBufferData + i * RowSizeInBytes, (BYTE*)MappedData + i * PlacedSubResourceFootPrint.Footprint.RowPitch, RowSizeInBytes);
	}

	OcclusionBufferTextureReadback[renderDevice.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);
}