// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ShadowResolvePass.h"

#include "MSAADepthBufferResolvePass.h"
#include "ShadowMapPass.h"

#include <Engine/Engine.h>

#include "../RenderSystem.h"

#undef SAFE_DX
#define SAFE_DX(Func) Func

struct ShadowResolveConstantBuffer
{
	XMMATRIX ReProjMatrices[4];
};

void ShadowResolvePass::Init(RenderSystem& renderSystem)
{
	CascadedShadowMapTextures[0] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTexture(0);
	CascadedShadowMapTextures[1] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTexture(1);
	CascadedShadowMapTextures[2] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTexture(2);
	CascadedShadowMapTextures[3] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTexture(3);

	ResolvedDepthBufferTextureSRV = ((MSAADepthBufferResolvePass*)renderSystem.GetRenderPass("MSAADepthBufferResolvePass"))->GetResolvedDepthBufferTextureSRV();
	CascadedShadowMapTexturesSRVs[0] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTextureSRV(0);
	CascadedShadowMapTexturesSRVs[1] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTextureSRV(1);
	CascadedShadowMapTexturesSRVs[2] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTextureSRV(2);
	CascadedShadowMapTexturesSRVs[3] = ((ShadowMapPass*)renderSystem.GetRenderPass("ShadowMapPass"))->GetCascadedShadowMapTextureSRV(3);

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	ShadowMaskTexture = renderSystem.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderSystem.GetResolutionWidth(), renderSystem.GetResolutionHeight(), DXGI_FORMAT::DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET, &ClearValue);

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	ShadowMaskTextureRTV = renderSystem.GetRTDescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateRenderTargetView(ShadowMaskTexture.DXTexture, &RTVDesc, ShadowMaskTextureRTV);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	ShadowMaskTextureSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(ShadowMaskTexture.DXTexture, &SRVDesc, ShadowMaskTextureSRV);

	GPUShadowResolveConstantBuffer = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr);

	CPUShadowResolveConstantBuffers[0] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
	CPUShadowResolveConstantBuffers[1] = renderSystem.CreateBuffer(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescBuffer(256), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUShadowResolveConstantBuffer.DXBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	ShadowResolveConstantBufferCBV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateConstantBufferView(&CBVDesc, ShadowResolveConstantBufferCBV);

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
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = ShadowResolvePixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ShadowResolvePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(ShadowResolvePipelineState)));

	ShadowResolveCBTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_CONSTANT_BUFFERS]);
	ShadowResolveSRTable = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);
}

void ShadowResolvePass::Execute(RenderSystem& renderSystem)
{
	renderSystem.SwitchResourceState(ShadowMaskTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	renderSystem.SwitchResourceState(*CascadedShadowMapTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(*CascadedShadowMapTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(*CascadedShadowMapTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(*CascadedShadowMapTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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

	SAFE_DX(CPUShadowResolveConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Map(0, &ReadRange, &ConstantBufferData));

	ShadowResolveConstantBuffer& ConstantBuffer = *((ShadowResolveConstantBuffer*)((BYTE*)ConstantBufferData));

	ConstantBuffer.ReProjMatrices[0] = ReProjMatrices[0];
	ConstantBuffer.ReProjMatrices[1] = ReProjMatrices[1];
	ConstantBuffer.ReProjMatrices[2] = ReProjMatrices[2];
	ConstantBuffer.ReProjMatrices[3] = ReProjMatrices[3];

	WrittenRange.Begin = 0;
	WrittenRange.End = 256;

	CPUShadowResolveConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer->Unmap(0, &WrittenRange);

	renderSystem.SwitchResourceState(GPUShadowResolveConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);

	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->CopyBufferRegion(GPUShadowResolveConstantBuffer.DXBuffer, 0, CPUShadowResolveConstantBuffers[renderSystem.GetCurrentFrameIndex()].DXBuffer, 0, 256);

	renderSystem.SwitchResourceState(GPUShadowResolveConstantBuffer, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	renderSystem.ApplyPendingBarriers();

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

	ShadowResolveCBTable[0] = ShadowResolveConstantBufferCBV;
	ShadowResolveCBTable.SetTableSize(1);
	ShadowResolveCBTable.UpdateDescriptorTable();

	ShadowResolveSRTable[0] = ResolvedDepthBufferTextureSRV;
	ShadowResolveSRTable[1] = CascadedShadowMapTexturesSRVs[0];
	ShadowResolveSRTable[2] = CascadedShadowMapTexturesSRVs[1];
	ShadowResolveSRTable[3] = CascadedShadowMapTexturesSRVs[2];
	ShadowResolveSRTable[4] = CascadedShadowMapTexturesSRVs[3];
	ShadowResolveSRTable.SetTableSize(5);
	ShadowResolveSRTable.UpdateDescriptorTable();

	renderSystem.GetCommandList()->DiscardResource(ShadowMaskTexture.DXTexture, nullptr);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SAMPLERS, renderSystem.GetShadowMapSamplerTable());

	renderSystem.GetCommandList()->SetPipelineState(ShadowResolvePipelineState);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_CONSTANT_BUFFERS, ShadowResolveCBTable);
	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, ShadowResolveSRTable);

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);
}