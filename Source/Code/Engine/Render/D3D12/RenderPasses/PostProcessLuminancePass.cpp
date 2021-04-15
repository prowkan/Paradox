// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessLuminancePass.h"

#include "HDRSceneColorResolvePass.h"

#include "../RenderDeviceD3D12.h"

#include <Engine/Engine.h>

#undef SAFE_DX
#define SAFE_DX(Func) Func

void PostProcessLuminancePass::Init(RenderDeviceD3D12& renderDevice)
{
	ResolvedHDRSceneColorTexture = ((HDRSceneColorResolvePass*)renderDevice.GetRenderPass("HDRSceneColorResolvePass"))->GetResolvedHDRSceneColorTexture();
	ResolvedHDRSceneColorTextureSRV = ((HDRSceneColorResolvePass*)renderDevice.GetRenderPass("HDRSceneColorResolvePass"))->GetResolvedHDRSceneColorTextureSRV();
	
	int Widths[4] = { 1280, 80, 5, 1 };
	int Heights[4] = { 720, 45, 3, 1 };

	for (int i = 0; i < 4; i++)
	{
		SceneLuminanceTextures[i] = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(Widths[i], Heights[i], DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SceneLuminanceTexturesUAVs[i] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

		renderDevice.GetDevice()->CreateUnorderedAccessView(SceneLuminanceTextures[i].DXTexture, nullptr, &UAVDesc, SceneLuminanceTexturesUAVs[i]);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SceneLuminanceTexturesSRVs[i] = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

		renderDevice.GetDevice()->CreateShaderResourceView(SceneLuminanceTextures[i].DXTexture, &SRVDesc, SceneLuminanceTexturesSRVs[i]);
	}

	AverageLuminanceTexture = renderDevice.CreateTexture(DX12Helpers::CreateDXHeapProperties(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, DX12Helpers::CreateDXResourceDescTexture2D(1, 1, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT, D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);

	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureUAV = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateUnorderedAccessView(AverageLuminanceTexture.DXTexture, nullptr, &UAVDesc, AverageLuminanceTextureUAV);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureSRV = renderDevice.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderDevice.GetDevice()->CreateShaderResourceView(AverageLuminanceTexture.DXTexture, &SRVDesc, AverageLuminanceTextureSRV);

	void *LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.LuminanceCalc");
	SIZE_T LuminanceCalcComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.LuminanceCalc");

	void *LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.LuminanceSum");
	SIZE_T LuminanceSumComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.LuminanceSum");

	void *LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetFileSystem().GetShaderData("ShaderModel51.LuminanceAvg");
	SIZE_T LuminanceAvgComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetShaderSize("ShaderModel51.LuminanceAvg");
	
	D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePipelineStateDesc;
	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceCalcComputeShaderByteCodeLength;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceCalcComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = renderDevice.GetComputeRootSignature();

	SAFE_DX(renderDevice.GetDevice()->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceCalcPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceSumComputeShaderByteCodeLength;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceSumComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = renderDevice.GetComputeRootSignature();

	SAFE_DX(renderDevice.GetDevice()->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceSumPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceAvgComputeShaderByteCodeLength;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceAvgComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = renderDevice.GetComputeRootSignature();

	SAFE_DX(renderDevice.GetDevice()->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceAvgPipelineState)));

	for (int i = 0; i < 5; i++)
	{
		LuminancePassSRTables[i] = renderDevice.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderDevice.GetComputeRootSignature().GetRootSignatureDesc().pParameters[RenderDeviceD3D12::COMPUTE_SHADER_SHADER_RESOURCES]);
		LuminancePassUATables[i] = renderDevice.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderDevice.GetComputeRootSignature().GetRootSignatureDesc().pParameters[RenderDeviceD3D12::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS]);
	}
}

void PostProcessLuminancePass::Execute(RenderDeviceD3D12& renderDevice)
{
	renderDevice.SwitchResourceState(*ResolvedHDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderDevice.SwitchResourceState(SceneLuminanceTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderDevice.SwitchResourceState(SceneLuminanceTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderDevice.SwitchResourceState(SceneLuminanceTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderDevice.SwitchResourceState(SceneLuminanceTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderDevice.SwitchResourceState(AverageLuminanceTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->DiscardResource(SceneLuminanceTextures[0].DXTexture, nullptr);
	renderDevice.GetCommandList()->DiscardResource(SceneLuminanceTextures[1].DXTexture, nullptr);
	renderDevice.GetCommandList()->DiscardResource(SceneLuminanceTextures[2].DXTexture, nullptr);
	renderDevice.GetCommandList()->DiscardResource(SceneLuminanceTextures[3].DXTexture, nullptr);
	renderDevice.GetCommandList()->DiscardResource(AverageLuminanceTexture.DXTexture, nullptr);

	renderDevice.GetCommandList()->SetPipelineState(LuminanceCalcPipelineState);

	LuminancePassUATables[0][0] = SceneLuminanceTexturesUAVs[0];
	LuminancePassSRTables[0].SetTableSize(1);
	LuminancePassUATables[0].UpdateDescriptorTable();

	LuminancePassSRTables[0][0] = ResolvedHDRSceneColorTextureSRV;
	LuminancePassUATables[0].SetTableSize(1);
	LuminancePassSRTables[0].UpdateDescriptorTable();

	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[0]);
	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[0]);

	renderDevice.GetCommandList()->Dispatch(80, 45, 1);

	renderDevice.SwitchResourceState(SceneLuminanceTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->SetPipelineState(LuminanceSumPipelineState);

	LuminancePassSRTables[1][0] = SceneLuminanceTexturesSRVs[0];
	LuminancePassSRTables[1].SetTableSize(1);
	LuminancePassSRTables[1].UpdateDescriptorTable();

	LuminancePassUATables[1][0] = SceneLuminanceTexturesUAVs[1];
	LuminancePassUATables[1].SetTableSize(1);
	LuminancePassUATables[1].UpdateDescriptorTable();

	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[1]);
	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[1]);

	renderDevice.GetCommandList()->Dispatch(80, 45, 1);

	renderDevice.SwitchResourceState(SceneLuminanceTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderDevice.ApplyPendingBarriers();

	LuminancePassSRTables[2][0] = SceneLuminanceTexturesSRVs[1];
	LuminancePassSRTables[2].SetTableSize(1);
	LuminancePassSRTables[2].UpdateDescriptorTable();

	LuminancePassUATables[2][0] = SceneLuminanceTexturesUAVs[2];
	LuminancePassUATables[2].SetTableSize(1);
	LuminancePassUATables[2].UpdateDescriptorTable();

	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[2]);
	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[2]);

	renderDevice.GetCommandList()->Dispatch(5, 3, 1);

	renderDevice.SwitchResourceState(SceneLuminanceTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderDevice.ApplyPendingBarriers();

	LuminancePassSRTables[3][0] = SceneLuminanceTexturesSRVs[2];
	LuminancePassSRTables[3].SetTableSize(1);
	LuminancePassSRTables[3].UpdateDescriptorTable();

	LuminancePassUATables[3][0] = SceneLuminanceTexturesUAVs[3];
	LuminancePassUATables[3].SetTableSize(1);
	LuminancePassUATables[3].UpdateDescriptorTable();

	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[3]);
	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[3]);
		
	renderDevice.GetCommandList()->Dispatch(1, 1, 1);

	renderDevice.SwitchResourceState(SceneLuminanceTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderDevice.ApplyPendingBarriers();

	renderDevice.GetCommandList()->SetPipelineState(LuminanceAvgPipelineState);

	LuminancePassUATables[4][0] = AverageLuminanceTextureUAV;
	LuminancePassSRTables[4].SetTableSize(1);
	LuminancePassUATables[4].UpdateDescriptorTable();

	LuminancePassSRTables[4][0] = SceneLuminanceTexturesSRVs[3];
	LuminancePassUATables[4].SetTableSize(1);
	LuminancePassSRTables[4].UpdateDescriptorTable();

	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[4]);
	renderDevice.GetCommandList()->SetComputeRootDescriptorTable(RenderDeviceD3D12::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[4]);
		
	renderDevice.GetCommandList()->Dispatch(1, 1, 1);
}