// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessBloomPass.h"

#include "HDRSceneColorResolvePass.h"
#include "PostProcessLuminancePass.h"

#include <Engine/Engine.h>

#undef SAFE_DX
#define SAFE_DX(Func) Func

void PostProcessBloomPass::Init(RenderSystem& renderSystem)
{
	ResolvedHDRSceneColorTextureSRV = ((HDRSceneColorResolvePass*)renderSystem.GetRenderPass("HDRSceneColorResolvePass"))->GetResolvedHDRSceneColorTextureSRV();
	SceneLuminanceTextureSRV = ((PostProcessLuminancePass*)renderSystem.GetRenderPass("PostProcessLuminancePass"))->GetSceneLuminanceTextureSRV();

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

	for (int i = 0; i < 7; i++)
	{
		BloomTextures[0][i] = renderSystem.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderSystem.GetResolutionWidth() >> i, renderSystem.GetResolutionHeight() >> i, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue);
		BloomTextures[1][i] = renderSystem.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderSystem.GetResolutionWidth() >> i, renderSystem.GetResolutionHeight() >> i, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue);
		BloomTextures[2][i] = renderSystem.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(renderSystem.GetResolutionWidth() >> i, renderSystem.GetResolutionHeight() >> i, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue);
	}

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 7; i++)
	{
		BloomTexturesRTVs[0][i] = renderSystem.GetRTDescriptorHeap().AllocateDescriptor();
		BloomTexturesRTVs[1][i] = renderSystem.GetRTDescriptorHeap().AllocateDescriptor();
		BloomTexturesRTVs[2][i] = renderSystem.GetRTDescriptorHeap().AllocateDescriptor();

		renderSystem.GetDevice()->CreateRenderTargetView(BloomTextures[0][i].DXTexture, &RTVDesc, BloomTexturesRTVs[0][i]);
		renderSystem.GetDevice()->CreateRenderTargetView(BloomTextures[1][i].DXTexture, &RTVDesc, BloomTexturesRTVs[1][i]);
		renderSystem.GetDevice()->CreateRenderTargetView(BloomTextures[2][i].DXTexture, &RTVDesc, BloomTexturesRTVs[2][i]);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 7; i++)
	{
		BloomTexturesSRVs[0][i] = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();
		BloomTexturesSRVs[1][i] = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();
		BloomTexturesSRVs[2][i] = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

		renderSystem.GetDevice()->CreateShaderResourceView(BloomTextures[0][i].DXTexture, &SRVDesc, BloomTexturesSRVs[0][i]);
		renderSystem.GetDevice()->CreateShaderResourceView(BloomTextures[1][i].DXTexture, &SRVDesc, BloomTexturesSRVs[1][i]);
		renderSystem.GetDevice()->CreateShaderResourceView(BloomTextures[2][i].DXTexture, &SRVDesc, BloomTexturesSRVs[2][i]);
	}

	SIZE_T BrightPassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("BrightPass");
	ScopedMemoryBlockArray<BYTE> BrightPassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(BrightPassPixelShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("BrightPass", BrightPassPixelShaderByteCodeData);

	SIZE_T ImageResamplePixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("ImageResample");
	ScopedMemoryBlockArray<BYTE> ImageResamplePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ImageResamplePixelShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("ImageResample", ImageResamplePixelShaderByteCodeData);

	SIZE_T HorizontalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("HorizontalBlur");
	ScopedMemoryBlockArray<BYTE> HorizontalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HorizontalBlurPixelShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("HorizontalBlur", HorizontalBlurPixelShaderByteCodeData);

	SIZE_T VerticalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("VerticalBlur");
	ScopedMemoryBlockArray<BYTE> VerticalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(VerticalBlurPixelShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("VerticalBlur", VerticalBlurPixelShaderByteCodeData);

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
	GraphicsPipelineStateDesc.PS.BytecodeLength = BrightPassPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = BrightPassPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(BrightPassPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = ImageResamplePixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ImageResamplePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DownSamplePipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = ImageResamplePixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ImageResamplePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(UpSampleWithAddBlendPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = HorizontalBlurPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = HorizontalBlurPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(HorizontalBlurPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = renderSystem.GetGraphicsRootSignature();
	GraphicsPipelineStateDesc.PS.BytecodeLength = VerticalBlurPixelShaderByteCodeLength;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = VerticalBlurPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS = renderSystem.GetFullScreenQuadVertexShader();

	SAFE_DX(renderSystem.GetDevice()->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(VerticalBlurPipelineState)));

	for (int i = 0; i < 3; i++)
	{
		BloomPassSRTables1[i] = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);
		BloomPassSRTables1[i].SetTableSize(i == 0 ? 2 : 1);
	}
	
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			BloomPassSRTables2[j][i] = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);
		}
	}

	for (int i = 0; i < 6; i++)
	{
		BloomPassSRTables3[i] = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetGraphicsRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::PIXEL_SHADER_SHADER_RESOURCES]);
	}
}

void PostProcessBloomPass::Execute(RenderSystem& renderSystem)
{
	for (int i = 0; i < 7; i++)
	{
		renderSystem.SwitchResourceState(BloomTextures[0][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
		renderSystem.SwitchResourceState(BloomTextures[1][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
		renderSystem.SwitchResourceState(BloomTextures[2][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	renderSystem.ApplyPendingBarriers();

	for (int i = 0; i < 7; i++)
	{
		renderSystem.GetCommandList()->DiscardResource(BloomTextures[0][i].DXTexture, nullptr);
		renderSystem.GetCommandList()->DiscardResource(BloomTextures[1][i].DXTexture, nullptr);
		renderSystem.GetCommandList()->DiscardResource(BloomTextures[2][i].DXTexture, nullptr);
	}

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &BloomTexturesRTVs[0][0], TRUE, nullptr);

	D3D12_VIEWPORT Viewport;
	Viewport.Height = FLOAT(renderSystem.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = FLOAT(renderSystem.GetResolutionWidth());

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	D3D12_RECT ScissorRect;
	ScissorRect.bottom = renderSystem.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderSystem.GetResolutionWidth();
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SAMPLERS, renderSystem.GetBiLinearSamplerTable());

	renderSystem.GetCommandList()->SetPipelineState(BrightPassPipelineState);

	BloomPassSRTables1[0][0] = ResolvedHDRSceneColorTextureSRV;
	BloomPassSRTables1[0][1] = SceneLuminanceTextureSRV;
	BloomPassSRTables1[0].SetTableSize(2);
	BloomPassSRTables1[0].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, BloomPassSRTables1[0]);

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);

	renderSystem.SwitchResourceState(BloomTextures[0][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &BloomTexturesRTVs[1][0], TRUE, nullptr);

	Viewport.Height = FLOAT(renderSystem.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = FLOAT(renderSystem.GetResolutionWidth());

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	ScissorRect.bottom = renderSystem.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderSystem.GetResolutionWidth();
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderSystem.GetCommandList()->SetPipelineState(HorizontalBlurPipelineState);

	BloomPassSRTables1[1][0] = BloomTexturesSRVs[0][0];
	BloomPassSRTables1[1].SetTableSize(1);
	BloomPassSRTables1[1].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, BloomPassSRTables1[1]);

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);

	renderSystem.SwitchResourceState(BloomTextures[1][0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	renderSystem.GetCommandList()->OMSetRenderTargets(1, &BloomTexturesRTVs[2][0], TRUE, nullptr);

	Viewport.Height = FLOAT(renderSystem.GetResolutionHeight());
	Viewport.MaxDepth = 1.0f;
	Viewport.MinDepth = 0.0f;
	Viewport.TopLeftX = 0.0f;
	Viewport.TopLeftY = 0.0f;
	Viewport.Width = FLOAT(renderSystem.GetResolutionWidth());

	renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

	ScissorRect.bottom = renderSystem.GetResolutionHeight();
	ScissorRect.left = 0;
	ScissorRect.right = renderSystem.GetResolutionWidth();
	ScissorRect.top = 0;

	renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

	renderSystem.GetCommandList()->SetPipelineState(VerticalBlurPipelineState);

	BloomPassSRTables1[2][0] = BloomTexturesSRVs[1][0];
	BloomPassSRTables1[2].SetTableSize(1);
	BloomPassSRTables1[2].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, BloomPassSRTables1[2]);

	renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);

	for (int i = 1; i < 7; i++)
	{
		renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		renderSystem.GetCommandList()->OMSetRenderTargets(1, &BloomTexturesRTVs[0][i], TRUE, nullptr);

		Viewport.Height = FLOAT(renderSystem.GetResolutionHeight() >> i);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(renderSystem.GetResolutionWidth() >> i);

		renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = renderSystem.GetResolutionHeight() >> i;
		ScissorRect.left = 0;
		ScissorRect.right = renderSystem.GetResolutionWidth() >> i;
		ScissorRect.top = 0;

		renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

		renderSystem.GetCommandList()->SetPipelineState(DownSamplePipelineState);

		BloomPassSRTables2[i - 1][0][0] = BloomTexturesSRVs[0][i - 1];
		BloomPassSRTables2[i - 1][0].SetTableSize(1);
		BloomPassSRTables2[i - 1][0].UpdateDescriptorTable();

		renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, BloomPassSRTables2[i - 1][0]);

		renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);

		renderSystem.SwitchResourceState(BloomTextures[0][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		renderSystem.ApplyPendingBarriers();

		renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		renderSystem.GetCommandList()->OMSetRenderTargets(1, &BloomTexturesRTVs[1][i], TRUE, nullptr);

		Viewport.Height = FLOAT(renderSystem.GetResolutionHeight() >> i);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(renderSystem.GetResolutionWidth() >> i);

		renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = renderSystem.GetResolutionHeight() >> i;
		ScissorRect.left = 0;
		ScissorRect.right = renderSystem.GetResolutionWidth() >> i;
		ScissorRect.top = 0;

		renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

		renderSystem.GetCommandList()->SetPipelineState(HorizontalBlurPipelineState);

		BloomPassSRTables2[i - 1][1][0] = BloomTexturesSRVs[0][i];
		BloomPassSRTables2[i - 1][1].SetTableSize(1);
		BloomPassSRTables2[i - 1][1].UpdateDescriptorTable();

		renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, BloomPassSRTables2[i - 1][1]);

		renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);

		renderSystem.SwitchResourceState(BloomTextures[1][i], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		renderSystem.ApplyPendingBarriers();

		renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		renderSystem.GetCommandList()->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

		Viewport.Height = FLOAT(renderSystem.GetResolutionHeight() >> i);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(renderSystem.GetResolutionWidth() >> i);

		renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = renderSystem.GetResolutionHeight() >> i;
		ScissorRect.left = 0;
		ScissorRect.right = renderSystem.GetResolutionWidth() >> i;
		ScissorRect.top = 0;

		renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

		renderSystem.GetCommandList()->SetPipelineState(VerticalBlurPipelineState);

		BloomPassSRTables2[i - 1][2][0] = BloomTexturesSRVs[1][i];
		BloomPassSRTables2[i - 1][2].SetTableSize(1);
		BloomPassSRTables2[i - 1][2].UpdateDescriptorTable();

		renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, BloomPassSRTables2[i - 1][2]);

		renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);
	}

	for (int i = 5; i >= 0; i--)
	{
		renderSystem.SwitchResourceState(BloomTextures[2][i + 1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		renderSystem.ApplyPendingBarriers();

		renderSystem.GetCommandList()->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		renderSystem.GetCommandList()->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

		Viewport.Height = FLOAT(renderSystem.GetResolutionHeight() >> i);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(renderSystem.GetResolutionWidth() >> i);

		renderSystem.GetCommandList()->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = renderSystem.GetResolutionHeight() >> i;
		ScissorRect.left = 0;
		ScissorRect.right = renderSystem.GetResolutionWidth() >> i;
		ScissorRect.top = 0;

		renderSystem.GetCommandList()->RSSetScissorRects(1, &ScissorRect);

		renderSystem.GetCommandList()->SetPipelineState(UpSampleWithAddBlendPipelineState);

		BloomPassSRTables3[5 - i][0] = BloomTexturesSRVs[2][i + 1];
		BloomPassSRTables3[5 - i].SetTableSize(1);
		BloomPassSRTables3[5 - i].UpdateDescriptorTable();

		renderSystem.GetCommandList()->SetGraphicsRootDescriptorTable(RenderSystem::PIXEL_SHADER_SHADER_RESOURCES, BloomPassSRTables3[5 - i]);

		renderSystem.GetCommandList()->DrawInstanced(4, 1, 0, 0);
	}
}