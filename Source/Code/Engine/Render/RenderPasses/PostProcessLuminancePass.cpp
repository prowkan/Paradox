// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessLuminancePass.h"

#include "HDRSceneColorResolvePass.h"

#include "../RenderSystem.h"

#include <Engine/Engine.h>

#undef SAFE_DX
#define SAFE_DX(Func) Func

void PostProcessLuminancePass::Init(RenderSystem& renderSystem)
{
	ResolvedHDRSceneColorTexture = renderSystem.GetRenderPass<HDRSceneColorResolvePass>()->GetResolvedHDRSceneColorTexture();
	ResolvedHDRSceneColorTextureSRV = renderSystem.GetRenderPass<HDRSceneColorResolvePass>()->GetResolvedHDRSceneColorTextureSRV();

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	//ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	//ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	int Widths[4] = { 1280, 80, 5, 1 };
	int Heights[4] = { 720, 45, 3, 1 };

	for (int i = 0; i < 4; i++)
	{
		ResourceDesc.Width = Widths[i];
		ResourceDesc.Height = Heights[i];

		SceneLuminanceTextures[i] = renderSystem.CreateTexture(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SceneLuminanceTexturesUAVs[i] = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

		renderSystem.GetDevice()->CreateUnorderedAccessView(SceneLuminanceTextures[i].DXTexture, nullptr, &UAVDesc, SceneLuminanceTexturesUAVs[i]);
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
		SceneLuminanceTexturesSRVs[i] = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

		renderSystem.GetDevice()->CreateShaderResourceView(SceneLuminanceTextures[i].DXTexture, &SRVDesc, SceneLuminanceTexturesSRVs[i]);
	}

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 1;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	AverageLuminanceTexture = renderSystem.CreateTexture(HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr);

	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureUAV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateUnorderedAccessView(AverageLuminanceTexture.DXTexture, nullptr, &UAVDesc, AverageLuminanceTextureUAV);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureSRV = renderSystem.GetCBSRUADescriptorHeap().AllocateDescriptor();

	renderSystem.GetDevice()->CreateShaderResourceView(AverageLuminanceTexture.DXTexture, &SRVDesc, AverageLuminanceTextureSRV);

	HANDLE LuminanceCalcComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceCalc.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceCalcComputeShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(LuminanceCalcComputeShaderFile, &LuminanceCalcComputeShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceCalcComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceCalcComputeShaderFile, LuminanceCalcComputeShaderByteCodeData, (DWORD)LuminanceCalcComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceCalcComputeShaderFile);

	HANDLE LuminanceSumComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceSum.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceSumComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceSumComputeShaderFile, &LuminanceSumComputeShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceSumComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceSumComputeShaderFile, LuminanceSumComputeShaderByteCodeData, (DWORD)LuminanceSumComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceSumComputeShaderFile);

	HANDLE LuminanceAvgComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceAvg.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceAvgComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceAvgComputeShaderFile, &LuminanceAvgComputeShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceAvgComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceAvgComputeShaderFile, LuminanceAvgComputeShaderByteCodeData, (DWORD)LuminanceAvgComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceAvgComputeShaderFile);

	D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePipelineStateDesc;
	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceCalcComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceCalcComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = renderSystem.GetComputeRootSignature();

	SAFE_DX(renderSystem.GetDevice()->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceCalcPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceSumComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceSumComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = renderSystem.GetComputeRootSignature();

	SAFE_DX(renderSystem.GetDevice()->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceSumPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceAvgComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceAvgComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = renderSystem.GetComputeRootSignature();

	SAFE_DX(renderSystem.GetDevice()->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceAvgPipelineState)));

	for (int i = 0; i < 5; i++)
	{
		LuminancePassSRTables[i] = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetComputeRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::COMPUTE_SHADER_SHADER_RESOURCES]);
		LuminancePassUATables[i] = renderSystem.GetFrameResourcesDescriptorHeap().AllocateDescriptorTable(renderSystem.GetComputeRootSignature().GetRootSignatureDesc().pParameters[RenderSystem::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS]);
	}
}

void PostProcessLuminancePass::Execute(RenderSystem& renderSystem)
{
	renderSystem.SwitchResourceState(*ResolvedHDRSceneColorTexture, 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(SceneLuminanceTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->SetPipelineState(LuminanceCalcPipelineState);

	LuminancePassUATables[0][0] = SceneLuminanceTexturesUAVs[0];
	LuminancePassSRTables[0].SetTableSize(1);
	LuminancePassUATables[0].UpdateDescriptorTable();

	LuminancePassUATables[0].SetTableSize(1);
	LuminancePassSRTables[0][0] = ResolvedHDRSceneColorTextureSRV;
	LuminancePassSRTables[0].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[0]);
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[0]);

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[0].DXTexture, nullptr);

	renderSystem.GetCommandList()->Dispatch(80, 45, 1);

	renderSystem.SwitchResourceState(SceneLuminanceTextures[0], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(SceneLuminanceTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->SetPipelineState(LuminanceSumPipelineState);

	LuminancePassSRTables[1][0] = SceneLuminanceTexturesSRVs[0];
	LuminancePassSRTables[1].SetTableSize(1);
	LuminancePassSRTables[1].UpdateDescriptorTable();

	LuminancePassUATables[1][0] = SceneLuminanceTexturesUAVs[1];
	LuminancePassUATables[1].SetTableSize(1);
	LuminancePassUATables[1].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[1]);
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[1]);

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[1].DXTexture, nullptr);

	renderSystem.GetCommandList()->Dispatch(80, 45, 1);

	renderSystem.SwitchResourceState(SceneLuminanceTextures[1], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(SceneLuminanceTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderSystem.ApplyPendingBarriers();

	LuminancePassSRTables[2][0] = SceneLuminanceTexturesSRVs[1];
	LuminancePassSRTables[2].SetTableSize(1);
	LuminancePassSRTables[2].UpdateDescriptorTable();

	LuminancePassUATables[2][0] = SceneLuminanceTexturesUAVs[2];
	LuminancePassUATables[2].SetTableSize(1);
	LuminancePassUATables[2].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[2]);
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[2]);

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[2].DXTexture, nullptr);

	renderSystem.GetCommandList()->Dispatch(5, 3, 1);

	renderSystem.SwitchResourceState(SceneLuminanceTextures[2], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderSystem.SwitchResourceState(SceneLuminanceTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	renderSystem.ApplyPendingBarriers();

	LuminancePassSRTables[3][0] = SceneLuminanceTexturesSRVs[2];
	LuminancePassSRTables[3].SetTableSize(1);
	LuminancePassSRTables[3].UpdateDescriptorTable();

	LuminancePassUATables[3][0] = SceneLuminanceTexturesUAVs[3];
	LuminancePassUATables[3].SetTableSize(1);
	LuminancePassUATables[3].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[3]);
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[3]);

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[3].DXTexture, nullptr);

	renderSystem.GetCommandList()->Dispatch(1, 1, 1);

	renderSystem.SwitchResourceState(SceneLuminanceTextures[3], 0, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	renderSystem.ApplyPendingBarriers();

	renderSystem.GetCommandList()->SetPipelineState(LuminanceAvgPipelineState);

	LuminancePassUATables[4][0] = AverageLuminanceTextureUAV;
	LuminancePassSRTables[4].SetTableSize(1);
	LuminancePassUATables[4].UpdateDescriptorTable();

	LuminancePassSRTables[4][0] = SceneLuminanceTexturesSRVs[3];
	LuminancePassUATables[4].SetTableSize(1);
	LuminancePassSRTables[4].UpdateDescriptorTable();

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_SHADER_RESOURCES, LuminancePassSRTables[4]);
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(RenderSystem::COMPUTE_SHADER_UNORDERED_ACCESS_VIEWS, LuminancePassUATables[4]);

	renderSystem.GetCommandList()->DiscardResource(AverageLuminanceTexture.DXTexture, nullptr);

	renderSystem.GetCommandList()->Dispatch(1, 1, 1);
}