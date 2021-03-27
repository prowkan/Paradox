// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ShadowResolvePass.h"

#include <Engine/Engine.h>

#include "../RenderSystem.h"

struct ShadowResolveConstantBuffer
{
	XMMATRIX ReProjMatrices[4];
};

void ShadowResolvePass::Init(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	ResourceDesc.Height = renderSystem.GetResolutionHeight();
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = renderSystem.GetResolutionWidth();

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(ShadowMaskTexture)));

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	ShadowMaskTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	RTDescriptorsCount++;

	Device->CreateRenderTargetView(ShadowMaskTexture, &RTVDesc, ShadowMaskTextureRTV);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	ShadowMaskTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(ShadowMaskTexture, &SRVDesc, ShadowMaskTextureSRV);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
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
	ResourceDesc.Width = 256;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUShadowResolveConstantBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUShadowResolveConstantBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUShadowResolveConstantBuffers[1])));

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUShadowResolveConstantBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	ShadowResolveConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateConstantBufferView(&CBVDesc, ShadowResolveConstantBufferCBV);

	HANDLE ShadowResolvePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ShadowResolve.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER ShadowResolvePixelShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(ShadowResolvePixelShaderFile, &ShadowResolvePixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> ShadowResolvePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowResolvePixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(ShadowResolvePixelShaderFile, ShadowResolvePixelShaderByteCodeData, (DWORD)ShadowResolvePixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(ShadowResolvePixelShaderFile);

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
	GraphicsPipelineStateDesc.PS.BytecodeLength = ShadowResolvePixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ShadowResolvePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(ShadowResolvePipelineState)));
}

void ShadowResolvePass::Execute(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_BARRIER ResourceBarriers[6];

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ShadowMaskTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = CascadedShadowMapTextures[0];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = CascadedShadowMapTextures[1];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[3].Transition.pResource = CascadedShadowMapTextures[2];
	ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[3].Transition.Subresource = 0;
	ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[4].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[4].Transition.pResource = CascadedShadowMapTextures[3];
	ResourceBarriers[4].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[4].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[4].Transition.Subresource = 0;
	ResourceBarriers[4].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	Camera& camera = gameFramework.GetCamera();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();

	XMMATRIX ShadowViewMatrices[4], ShadowProjMatrices[4], ShadowViewProjMatrices[4];

	ShadowViewMatrices[0] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 10.0f, CameraLocation.y + 10.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[1] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 20.0f, CameraLocation.y + 20.0f, CameraLocation.z - 20.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[2] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 50.0f, CameraLocation.y + 50.0f, CameraLocation.z - 50.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[3] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 100.0f, CameraLocation.y + 100.0f, CameraLocation.z - 100.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ShadowProjMatrices[0] = XMMatrixOrthographicLH(10.0f, 10.0f, 0.01f, 500.0f);
	ShadowProjMatrices[1] = XMMatrixOrthographicLH(20.0f, 20.0f, 0.01f, 500.0f);
	ShadowProjMatrices[2] = XMMatrixOrthographicLH(50.0f, 50.0f, 0.01f, 500.0f);
	ShadowProjMatrices[3] = XMMatrixOrthographicLH(100.0f, 100.0f, 0.01f, 500.0f);

	ShadowViewProjMatrices[0] = ShadowViewMatrices[0] * ShadowProjMatrices[0];
	ShadowViewProjMatrices[1] = ShadowViewMatrices[1] * ShadowProjMatrices[1];
	ShadowViewProjMatrices[2] = ShadowViewMatrices[2] * ShadowProjMatrices[2];
	ShadowViewProjMatrices[3] = ShadowViewMatrices[3] * ShadowProjMatrices[3];

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	XMMATRIX ReProjMatrices[4];
	ReProjMatrices[0] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[0];
	ReProjMatrices[1] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[1];
	ReProjMatrices[2] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[2];
	ReProjMatrices[3] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[3];

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;

	void *ConstantBufferData;

	SAFE_DX(CPUShadowResolveConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

	ShadowResolveConstantBuffer& ConstantBuffer = *((ShadowResolveConstantBuffer*)((BYTE*)ConstantBufferData));

	ConstantBuffer.ReProjMatrices[0] = ReProjMatrices[0];
	ConstantBuffer.ReProjMatrices[1] = ReProjMatrices[1];
	ConstantBuffer.ReProjMatrices[2] = ReProjMatrices[2];
	ConstantBuffer.ReProjMatrices[3] = ReProjMatrices[3];

	WrittenRange.Begin = 0;
	WrittenRange.End = 256;

	CPUShadowResolveConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

	ResourceBarriers[5].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[5].Transition.pResource = GPUShadowResolveConstantBuffer;
	ResourceBarriers[5].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[5].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarriers[5].Transition.Subresource = 0;
	ResourceBarriers[5].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(6, ResourceBarriers);

	renderSystem.GetCommandList()->CopyBufferRegion(GPUShadowResolveConstantBuffer, 0, CPUShadowResolveConstantBuffers[CurrentFrameIndex], 0, 256);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = GPUShadowResolveConstantBuffer;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(1, ResourceBarriers);

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &ShadowMaskTextureRTV, TRUE, nullptr);

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

	renderSystem.GetCommandList()->DiscardResource(ShadowMaskTexture, nullptr);

	Device->CopyDescriptorsSimple(1, SamplerCPUHandle, ShadowMapSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SamplerCPUHandle.ptr += SamplerHandleSize;

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(5, D3D12_GPU_DESCRIPTOR_HANDLE{ SamplerGPUHandle.ptr + 0 * ResourceHandleSize });
	SamplerGPUHandle.ptr += SamplerHandleSize;

	UINT DestRangeSize = 6;
	UINT SourceRangeSizes[7] = { 1, 1, 4 };
	D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[7] = { ShadowResolveConstantBufferCBV, ResolvedDepthBufferTextureSRV, CascadedShadowMapTexturesSRVs[0] };

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 3, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 6 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetPipelineState(ShadowResolvePipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

	ResourceGPUHandle.ptr += 6 * ResourceHandleSize;

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);
}