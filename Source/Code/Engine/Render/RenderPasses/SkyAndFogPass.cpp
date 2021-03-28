// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SkyAndFogPass.h"

#include "GBufferOpaquePass.h"
#include "DeferredLightingPass.h"

#include "../RenderSystem.h"

#include <Engine/Engine.h>

#undef SAFE_DX
#define SAFE_DX(Func) Func

struct SkyConstantBuffer
{
	XMMATRIX WVPMatrix;
};

struct SunConstantBuffer
{
	XMMATRIX ViewMatrix;
	XMMATRIX ProjMatrix;
	XMFLOAT3 SunPosition;
};

void SkyAndFogPass::Init(RenderSystem& renderSystem)
{
	HDRSceneColorTextureRTV = renderSystem.GetRenderPass<DeferredLightingPass>()->GetHDRSceneColorTextureRTV();

	DepthBufferTexture = renderSystem.GetRenderPass<GBufferOpaquePass>()->GetDepthBufferTexture();
	DepthBufferTextureDSV = renderSystem.GetRenderPass<GBufferOpaquePass>()->GetDepthBufferTextureDSV();
	DepthBufferTextureSRV = renderSystem.GetRenderPass<GBufferOpaquePass>()->GetDepthBufferTextureSRV();

	UINT SkyMeshVertexCount = 1 + 25 * 100 + 1;
	UINT SkyMeshIndexCount = 300 + 24 * 600 + 300;

	Vertex SkyMeshVertices[1 + 25 * 100 + 1];
	WORD SkyMeshIndices[300 + 24 * 600 + 300];

	SkyMeshVertices[0].Position = XMFLOAT3(0.0f, 1.0f, 0.0f);
	SkyMeshVertices[0].TexCoord = XMFLOAT2(0.5f, 0.5f);

	SkyMeshVertices[1 + 25 * 100].Position = XMFLOAT3(0.0f, -1.0f, 0.0f);
	SkyMeshVertices[1 + 25 * 100].TexCoord = XMFLOAT2(0.5f, 0.5f);

	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 25; j++)
		{
			float Theta = ((j + 1) / 25.0f) * 0.5f * 3.1416f;
			float Phi = (i / 100.0f) * 2.0f * 3.1416f;

			float X = sinf(Theta) * cosf(Phi);
			float Y = sinf(Theta) * sinf(Phi);
			float Z = cosf(Theta);

			SkyMeshVertices[1 + j * 100 + i].Position = XMFLOAT3(X, Z, Y);
			SkyMeshVertices[1 + j * 100 + i].TexCoord = XMFLOAT2(X * 0.5f + 0.5f, Y * 0.5f + 0.5f);
		}
	}

	for (int i = 0; i < 100; i++)
	{
		SkyMeshIndices[3 * i] = 0;
		SkyMeshIndices[3 * i + 1] = i + 1;
		SkyMeshIndices[3 * i + 2] = i != 99 ? i + 2 : 1;
	}

	for (int j = 0; j < 24; j++)
	{
		for (int i = 0; i < 100; i++)
		{
			SkyMeshIndices[300 + j * 600 + 6 * i] = 1 + i + j * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 1] = 1 + i + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 2] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 3] = 1 + i + j * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 4] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 5] = i != 99 ? 1 + i + 1 + j * 100 : 1 + j * 100;
		}
	}

	for (int i = 0; i < 100; i++)
	{
		SkyMeshIndices[300 + 24 * 600 + 3 * i] = 1 + 25 * 100;
		SkyMeshIndices[300 + 24 * 600 + 3 * i + 1] = i != 99 ? 1 + 24 * 100 + i + 1 : 1 + 24 * 100;
		SkyMeshIndices[300 + 24 * 600 + 3 * i + 2] = 1 + 24 * 100 + i;
	}

	D3D12_RESOURCE_DESC ResourceDesc = DX12Helpers::CreateDXResourceDescBuffer(sizeof(Vertex) * SkyMeshVertexCount);
	D3D12_HEAP_PROPERTIES HeapProperties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

	SAFE_DX(renderSystem.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyVertexBuffer)));

	ResourceDesc = DX12Helpers::CreateDXResourceDescBuffer(sizeof(WORD) * SkyMeshIndexCount);
	HeapProperties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

	SAFE_DX(renderSystem.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyIndexBuffer)));

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * SkyMeshVertexCount;

	SAFE_DX(renderSystem.GetUploadBuffer()->Map(0, &ReadRange, &MappedData));
	memcpy((BYTE*)MappedData, SkyMeshVertices, sizeof(Vertex) * SkyMeshVertexCount);
	memcpy((BYTE*)MappedData + sizeof(Vertex) * SkyMeshVertexCount, SkyMeshIndices, sizeof(WORD) * SkyMeshIndexCount);
	renderSystem.GetUploadBuffer()->Unmap(0, &WrittenRange);

	SAFE_DX(renderSystem.GetCommandAllocator(0)->Reset());
	SAFE_DX(renderSystem.GetCommandList()->Reset(renderSystem.GetCommandAllocator(0), nullptr));

	renderSystem.GetCommandList()->CopyBufferRegion(SkyVertexBuffer, 0, renderSystem.GetUploadBuffer(), 0, sizeof(Vertex) * SkyMeshVertexCount);
	renderSystem.GetCommandList()->CopyBufferRegion(SkyIndexBuffer, 0, renderSystem.GetUploadBuffer(), sizeof(Vertex) * SkyMeshVertexCount, sizeof(WORD) * SkyMeshIndexCount);

	D3D12_RESOURCE_BARRIER ResourceBarriers[2];
	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SkyVertexBuffer;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = SkyIndexBuffer;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	SAFE_DX(renderSystem.GetCommandList()->Close());

	renderSystem.GetCommandQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)&renderSystem.GetCommandList());

	SAFE_DX(renderSystem.GetCommandQueue()->Signal(renderSystem.GetCopySyncFence(), 1));

	if (renderSystem.GetCopySyncFence()->GetCompletedValue() != 1)
	{
		SAFE_DX(renderSystem.GetCopySyncFence()->SetEventOnCompletion(1, renderSystem.GetCopySyncEvent()));
		DWORD WaitResult = WaitForSingleObject(renderSystem.GetCopySyncEvent(), INFINITE);
	}

	SAFE_DX(renderSystem.GetCopySyncFence()->Signal(0));

	SkyVertexBufferAddress = SkyVertexBuffer->GetGPUVirtualAddress();
	SkyIndexBufferAddress = SkyIndexBuffer->GetGPUVirtualAddress();

	GPUSkyConstantBuffer = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);

	CPUSkyConstantBuffers[0] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUSkyConstantBuffers[1] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUSkyConstantBuffer.DXBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	SkyConstantBufferCBV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateConstantBufferView(&CBVDesc, SkyConstantBufferCBV);

	HANDLE SkyVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SkyVertexShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(SkyVertexShaderFile, &SkyVertexShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SkyVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(SkyVertexShaderFile, SkyVertexShaderByteCodeData, (DWORD)SkyVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SkyVertexShaderFile);

	HANDLE SkyPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SkyPixelShaderByteCodeLength;
	Result = GetFileSizeEx(SkyPixelShaderFile, &SkyPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SkyPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(SkyPixelShaderFile, SkyPixelShaderByteCodeData, (DWORD)SkyPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SkyPixelShaderFile);

	D3D12_INPUT_ELEMENT_DESC InputElementDescs[5];
	ZeroMemory(InputElementDescs, 5 * sizeof(D3D12_INPUT_ELEMENT_DESC));
	InputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 0;
	InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";
	InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[2].InputSlot = 0;
	InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[2].InstanceDataStepRate = 0;
	InputElementDescs[2].SemanticIndex = 0;
	InputElementDescs[2].SemanticName = "NORMAL";
	InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[3].InputSlot = 0;
	InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[3].InstanceDataStepRate = 0;
	InputElementDescs[3].SemanticIndex = 0;
	InputElementDescs[3].SemanticName = "TANGENT";
	InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[4].InputSlot = 0;
	InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[4].InstanceDataStepRate = 0;
	InputElementDescs[4].SemanticIndex = 0;
	InputElementDescs[4].SemanticName = "BINORMAL";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = SkyPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = SkyPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = SkyVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = SkyVertexShaderByteCodeData;

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SkyPipelineState)));

	ScopedMemoryBlockArray<Texel> SkyTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(2048 * 2048);

	for (int y = 0; y < 2048; y++)
	{
		for (int x = 0; x < 2048; x++)
		{
			float X = (x / 2048.0f) * 2.0f - 1.0f;
			float Y = (y / 2048.0f) * 2.0f - 1.0f;

			float D = sqrtf(X * X + Y * Y);

			SkyTextureTexels[y * 2048 + x].R = 0;
			SkyTextureTexels[y * 2048 + x].G = 128 * D < 255 ? BYTE(128 * D) : 255;
			SkyTextureTexels[y * 2048 + x].B = 255;
			SkyTextureTexels[y * 2048 + x].A = 255;
		}
	}

	ResourceDesc = DX12Helpers::CreateDXResourceDescTexture2D(2048, 2048, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	HeapProperties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

	SAFE_DX(renderSystem.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyTexture)));

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

	UINT NumRows;
	UINT64 RowSizeInBytes, TotalBytes;

	renderSystem.GetDevice()->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = TotalBytes;

	SAFE_DX(renderSystem.GetUploadBuffer()->Map(0, &ReadRange, &MappedData));

	BYTE *TexelData = (BYTE*)SkyTextureTexels;

	for (UINT j = 0; j < NumRows; j++)
	{
		memcpy((BYTE*)MappedData + PlacedSubResourceFootPrint.Offset + j * PlacedSubResourceFootPrint.Footprint.RowPitch, (BYTE*)TexelData + j * RowSizeInBytes, RowSizeInBytes);
	}

	renderSystem.GetUploadBuffer()->Unmap(0, &WrittenRange);

	SAFE_DX(renderSystem.GetCommandAllocator(0)->Reset());
	SAFE_DX(renderSystem.GetCommandList()->Reset(renderSystem.GetCommandAllocator(0), nullptr));

	D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

	SourceTextureCopyLocation.pResource = renderSystem.GetUploadBuffer();
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	DestTextureCopyLocation.pResource = SkyTexture;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
	DestTextureCopyLocation.SubresourceIndex = 0;

	renderSystem.GetCommandList()->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

	D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = SkyTexture;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(1, &ResourceBarrier);

	SAFE_DX(renderSystem.GetCommandList()->Close());

	renderSystem.GetCommandQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)&renderSystem.GetCommandList());

	SAFE_DX(renderSystem.GetCommandQueue()->Signal(renderSystem.GetCopySyncFence(), 1));

	if (renderSystem.GetCopySyncFence()->GetCompletedValue() != 1)
	{
		SAFE_DX(renderSystem.GetCopySyncFence()->SetEventOnCompletion(1, renderSystem.GetCopySyncEvent()));
		DWORD WaitResult = WaitForSingleObject(renderSystem.GetCopySyncEvent(), INFINITE);
	}

	SAFE_DX(renderSystem.GetCopySyncFence()->Signal(0));

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	SkyTextureSRV = renderSystem.GetTexturesDescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(SkyTexture, &SRVDesc, SkyTextureSRV);

	UINT SunMeshVertexCount = 4;
	UINT SunMeshIndexCount = 6;

	Vertex SunMeshVertices[4] = {

		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }

	};

	WORD SunMeshIndices[6] = { 0, 1, 2, 2, 1, 3 };

	COMRCPtr<ID3D12Resource> TemporarySunVertexBuffer, TemporarySunIndexBuffer;

	ResourceDesc = DX12Helpers::CreateDXResourceDescBuffer(sizeof(Vertex) * SunMeshVertexCount);
	HeapProperties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

	SAFE_DX(renderSystem.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunVertexBuffer)));

	ResourceDesc = DX12Helpers::CreateDXResourceDescBuffer(sizeof(WORD) * SunMeshIndexCount);
	HeapProperties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

	SAFE_DX(renderSystem.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunIndexBuffer)));

	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * SunMeshVertexCount + sizeof(WORD) * SunMeshIndexCount;

	SAFE_DX(renderSystem.GetUploadBuffer()->Map(0, &ReadRange, &MappedData));
	memcpy((BYTE*)MappedData, SunMeshVertices, sizeof(Vertex) * SunMeshVertexCount);
	memcpy((BYTE*)MappedData + sizeof(Vertex) * SunMeshVertexCount, SunMeshIndices, sizeof(WORD) * SunMeshIndexCount);
	renderSystem.GetUploadBuffer()->Unmap(0, &WrittenRange);

	SAFE_DX(renderSystem.GetCommandAllocator(0)->Reset());
	SAFE_DX(renderSystem.GetCommandList()->Reset(renderSystem.GetCommandAllocator(0), nullptr));

	renderSystem.GetCommandList()->CopyBufferRegion(SunVertexBuffer, 0, renderSystem.GetUploadBuffer(), 0, sizeof(Vertex) * SunMeshVertexCount);
	renderSystem.GetCommandList()->CopyBufferRegion(SunIndexBuffer, 0, renderSystem.GetUploadBuffer(), sizeof(Vertex) * SunMeshVertexCount, sizeof(WORD) * SunMeshIndexCount);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SunVertexBuffer;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = SunIndexBuffer;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	SAFE_DX(renderSystem.GetCommandList()->Close());

	renderSystem.GetCommandQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)&renderSystem.GetCommandList());

	SAFE_DX(renderSystem.GetCommandQueue()->Signal(renderSystem.GetCopySyncFence(), 1));

	if (renderSystem.GetCopySyncFence()->GetCompletedValue() != 1)
	{
		SAFE_DX(renderSystem.GetCopySyncFence()->SetEventOnCompletion(1, renderSystem.GetCopySyncEvent()));
		DWORD WaitResult = WaitForSingleObject(renderSystem.GetCopySyncEvent(), INFINITE);
	}

	SAFE_DX(renderSystem.GetCopySyncFence()->Signal(0));

	SunVertexBufferAddress = SunVertexBuffer->GetGPUVirtualAddress();
	SunIndexBufferAddress = SunIndexBuffer->GetGPUVirtualAddress();

	GPUSunConstantBuffer = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);

	CPUSunConstantBuffers[0] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUSunConstantBuffers[1] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	CBVDesc.BufferLocation = GPUSunConstantBuffer.DXBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	SunConstantBufferCBV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateConstantBufferView(&CBVDesc, SunConstantBufferCBV);

	HANDLE SunVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SunVertexShaderByteCodeLength;
	Result = GetFileSizeEx(SunVertexShaderFile, &SunVertexShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SunVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(SunVertexShaderFile, SunVertexShaderByteCodeData, (DWORD)SunVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SunVertexShaderFile);

	HANDLE SunPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SunPixelShaderByteCodeLength;
	Result = GetFileSizeEx(SunPixelShaderFile, &SunPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SunPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(SunPixelShaderFile, SunPixelShaderByteCodeData, (DWORD)SunPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SunPixelShaderFile);

	ZeroMemory(InputElementDescs, 5 * sizeof(D3D12_INPUT_ELEMENT_DESC));
	InputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 0;
	InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";
	InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[2].InputSlot = 0;
	InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[2].InstanceDataStepRate = 0;
	InputElementDescs[2].SemanticIndex = 0;
	InputElementDescs[2].SemanticName = "NORMAL";
	InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[3].InputSlot = 0;
	InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[3].InstanceDataStepRate = 0;
	InputElementDescs[3].SemanticIndex = 0;
	InputElementDescs[3].SemanticName = "TANGENT";
	InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[4].InputSlot = 0;
	InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[4].InstanceDataStepRate = 0;
	InputElementDescs[4].SemanticIndex = 0;
	InputElementDescs[4].SemanticName = "BINORMAL";

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = SunPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = SunPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = SunVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = SunVertexShaderByteCodeData;

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SunPipelineState)));

	ScopedMemoryBlockArray<Texel> SunTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(512 * 512);

	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			float X = (x / 512.0f) * 2.0f - 1.0f;
			float Y = (y / 512.0f) * 2.0f - 1.0f;

			float D = sqrtf(X * X + Y * Y);

			SunTextureTexels[y * 512 + x].R = 255;
			SunTextureTexels[y * 512 + x].G = 255;
			SunTextureTexels[y * 512 + x].B = 127 + 128 * D < 255 ? BYTE(127 + 128 * D) : 255;
			SunTextureTexels[y * 512 + x].A = 255 * D < 255 ? BYTE(255 * D) : 255;
		}
	}

	ResourceDesc = DX12Helpers::CreateDXResourceDescTexture2D(512, 512, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	HeapProperties = DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT);

	SAFE_DX(renderSystem.GetDevice()->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunTexture)));

	renderSystem.GetDevice()->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = TotalBytes;

	SAFE_DX(renderSystem.GetUploadBuffer()->Map(0, &ReadRange, &MappedData));

	TexelData = (BYTE*)SunTextureTexels;

	for (UINT j = 0; j < NumRows; j++)
	{
		memcpy((BYTE*)MappedData + PlacedSubResourceFootPrint.Offset + j * PlacedSubResourceFootPrint.Footprint.RowPitch, (BYTE*)TexelData + j * RowSizeInBytes, RowSizeInBytes);
	}

	renderSystem.GetUploadBuffer()->Unmap(0, &WrittenRange);

	SAFE_DX(renderSystem.GetCommandAllocator(0)->Reset());
	SAFE_DX(renderSystem.GetCommandList()->Reset(renderSystem.GetCommandAllocator(0), nullptr));

	SourceTextureCopyLocation.pResource = renderSystem.GetUploadBuffer();
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	DestTextureCopyLocation.pResource = SunTexture;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
	DestTextureCopyLocation.SubresourceIndex = 0;

	renderSystem.GetCommandList()->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = SunTexture;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(1, &ResourceBarrier);

	SAFE_DX(renderSystem.GetCommandList()->Close());

	renderSystem.GetCommandQueue()->ExecuteCommandLists(1, (ID3D12CommandList**)&renderSystem.GetCommandList());

	SAFE_DX(renderSystem.GetCommandQueue()->Signal(renderSystem.GetCopySyncFence(), 1));

	if (renderSystem.GetCopySyncFence()->GetCompletedValue() != 1)
	{
		SAFE_DX(renderSystem.GetCopySyncFence()->SetEventOnCompletion(1, renderSystem.GetCopySyncEvent()));
		DWORD WaitResult = WaitForSingleObject(renderSystem.GetCopySyncEvent(), INFINITE);
	}

	SAFE_DX(renderSystem.GetCopySyncFence()->Signal(0));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	SunTextureSRV = renderSystem.GetTexturesDescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(SunTexture, &SRVDesc, SunTextureSRV);

	HANDLE FogPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/Fog.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER FogPixelShaderByteCodeLength;
	Result = GetFileSizeEx(FogPixelShaderFile, &FogPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> FogPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FogPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(FogPixelShaderFile, FogPixelShaderByteCodeData, (DWORD)FogPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(FogPixelShaderFile);

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = FogPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = FogPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(FogPipelineState)));

	FogSRTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);

	SkyCBTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::VERTEX_SHADER_CONSTANT_BUFFERS]);
	SkySRTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);

	SunCBTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::VERTEX_SHADER_CONSTANT_BUFFERS]);
	SunSRTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);
}

void SkyAndFogPass::Execute(RenderSystem& renderSystem)
{
	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	Camera& camera = gameFramework.GetCamera();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	XMMATRIX SkyWorldMatrix = XMMatrixScaling(900.0f, 900.0f, 900.0f) * XMMatrixTranslation(CameraLocation.x, CameraLocation.y, CameraLocation.z);
	XMMATRIX SkyWVPMatrix = SkyWorldMatrix * ViewProjMatrix;

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;

	void *ConstantBufferData;

	SAFE_DX(CPUSkyConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

	SkyConstantBuffer& skyConstantBuffer = *((SkyConstantBuffer*)((BYTE*)ConstantBufferData));

	skyConstantBuffer.WVPMatrix = SkyWVPMatrix;

	WrittenRange.Begin = 0;
	WrittenRange.End = 256;

	CPUSkyConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);

	SAFE_DX(CPUSunConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

	SunConstantBuffer& sunConstantBuffer = *((SunConstantBuffer*)((BYTE*)ConstantBufferData));

	sunConstantBuffer.ViewMatrix = ViewMatrix;
	sunConstantBuffer.ProjMatrix = ProjMatrix;
	sunConstantBuffer.SunPosition = SunPosition;

	WrittenRange.Begin = 0;
	WrittenRange.End = 256;

	CPUSunConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	renderSystem.SwitchResourceState(GPUSkyConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	renderSystem.SwitchResourceState(GPUSunConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->CopyBufferRegion(GPUSkyConstantBuffer.DXBuffer, 0, CPUSkyConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, 256);
	renderSystem.GetCommandList()->CopyBufferRegion(GPUSunConstantBuffer.DXBuffer, 0, CPUSunConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, 256);

	renderSystem.SwitchResourceState(GPUSkyConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	renderSystem.SwitchResourceState(GPUSunConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = float(renderSystem.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(renderSystem.GetResolutionWidth());

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = renderSystem.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderSystem.GetResolutionWidth();
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	FogSRTable[0] = DepthBufferTextureSRV;
	FogSRTable.SetTableSize(1);
	FogSRTable.UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetPipelineState(FogPipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, FogSRTable);

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);

	renderSystem.SwitchResourceState(*DepthBufferTexture, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, &DepthBufferTextureDSV);

	Viewport.Height = float(renderSystem.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = float(renderSystem.GetResolutionWidth());

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	ScissorRect.bottom = renderSystem.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderSystem.GetResolutionWidth();
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SAMPLERS, renderSystem.GetTextureSamplerTable());

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
	VertexBufferView.BufferLocation = SkyVertexBufferAddress;
	VertexBufferView.SizeInBytes = sizeof(Vertex) * (1 + 25 * 100 + 1);
	VertexBufferView.StrideInBytes = sizeof(Vertex);

	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	IndexBufferView.BufferLocation = SkyIndexBufferAddress;
	IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	IndexBufferView.SizeInBytes = sizeof(WORD) * (300 + 24 * 600 + 300);

	SkyCBTable[0] = SkyConstantBufferCBV;
	SkyCBTable.SetTableSize(1);
	SkyCBTable.UpdateDescriptorTable();

	SkySRTable[0] = SkyTextureSRV;
	SkySRTable.SetTableSize(1);
	SkySRTable.UpdateDescriptorTable();

	renderSystem.GetCommandList()->IASetVertexBuffers(0, 1, &VertexBufferView);
	renderSystem.GetCommandList()->IASetIndexBuffer(&IndexBufferView);

	renderSystem.GetCommandList()->SetPipelineState(SkyPipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::VERTEX_SHADER_CONSTANT_BUFFERS, SkyCBTable);
	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, SkySRTable);

	renderSystem.GetCommandList()->DrawIndexedInstanced(300 + 24 * 600 + 300, 1, 0, 0, 0);

	VertexBufferView.BufferLocation = SunVertexBufferAddress;
	VertexBufferView.SizeInBytes = sizeof(Vertex) * 4;
	VertexBufferView.StrideInBytes = sizeof(Vertex);

	IndexBufferView.BufferLocation = SunIndexBufferAddress;
	IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	IndexBufferView.SizeInBytes = sizeof(WORD) * 6;

	SunCBTable[0] = SunConstantBufferCBV;
	SunCBTable.SetTableSize(1);
	SunCBTable.UpdateDescriptorTable();

	SunSRTable[0] = SunTextureSRV;
	SunSRTable.SetTableSize(1);
	SunSRTable.UpdateDescriptorTable();

	renderSystem.GetCommandList()->IASetVertexBuffers(0, 1, &VertexBufferView);
	renderSystem.GetCommandList()->IASetIndexBuffer(&IndexBufferView);

	renderSystem.GetCommandList()->SetPipelineState(SunPipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::VERTEX_SHADER_CONSTANT_BUFFERS, SunCBTable);
	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, SunSRTable);

	renderSystem.GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}