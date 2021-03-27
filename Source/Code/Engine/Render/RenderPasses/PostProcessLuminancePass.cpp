// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessLuminancePass.h"

#include "../RenderSystem.h"

#include <Engine/Engine.h>

void PostProcessLuminancePass::Init(RenderSystem& renderSystem)
{
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

		SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(SceneLuminanceTextures[i])));
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SceneLuminanceTexturesUAVs[i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateUnorderedAccessView(SceneLuminanceTextures[i], nullptr, &UAVDesc, SceneLuminanceTexturesUAVs[i]);
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
		SceneLuminanceTexturesSRVs[i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(SceneLuminanceTextures[i], &SRVDesc, SceneLuminanceTexturesSRVs[i]);
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

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, UUIDOF(AverageLuminanceTexture)));

	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureUAV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateUnorderedAccessView(AverageLuminanceTexture, nullptr, &UAVDesc, AverageLuminanceTextureUAV);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(AverageLuminanceTexture, &SRVDesc, AverageLuminanceTextureSRV);

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
	ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

	SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceCalcPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceSumComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceSumComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

	SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceSumPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceAvgComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceAvgComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

	SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceAvgPipelineState)));
}

void PostProcessLuminancePass::Execute(RenderSystem& renderSystem)
{
	D3D12_RESOURCE_BARRIER ResourceBarriers[3];

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = HDRSceneColorTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ResolvedHDRSceneColorTexture;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = SceneLuminanceTextures[0];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(3, ResourceBarriers);

	UINT DestRangeSize = 2;
	UINT SourceRangeSizes[2] = { 1, 1 };
	D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { ResolvedHDRSceneColorTextureSRV, SceneLuminanceTexturesUAVs[0] };

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetPipelineState(LuminanceCalcPipelineState);

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[0], nullptr);

	ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->Dispatch(80, 45, 1);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[1];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	DestRangeSize = 2;
	SourceRangeSizes[0] = 1;
	SourceRangeSizes[1] = 1;
	SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[0];
	SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[1];

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetPipelineState(LuminanceSumPipelineState);

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[1], nullptr);

	ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->Dispatch(80, 45, 1);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[1];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[2];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	DestRangeSize = 2;
	SourceRangeSizes[0] = 1;
	SourceRangeSizes[1] = 1;
	SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[1];
	SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[2];

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[2], nullptr);

	ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->Dispatch(5, 3, 1);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[2];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[3];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(2, ResourceBarriers);

	DestRangeSize = 2;
	SourceRangeSizes[0] = 1;
	SourceRangeSizes[1] = 1;
	SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[2];
	SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[3];

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

	renderSystem.GetCommandList()->DiscardResource(SceneLuminanceTextures[3], nullptr);

	ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->Dispatch(1, 1, 1);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[3];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	renderSystem.GetCommandList()->ResourceBarrier(1, ResourceBarriers);

	DestRangeSize = 2;
	SourceRangeSizes[0] = 1;
	SourceRangeSizes[1] = 1;
	SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[3];
	SourceCPUHandles[1] = AverageLuminanceTextureUAV;

	Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->SetPipelineState(LuminanceAvgPipelineState);

	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
	renderSystem.GetCommandList()->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

	renderSystem.GetCommandList()->DiscardResource(AverageLuminanceTexture, nullptr);

	ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

	renderSystem.GetCommandList()->Dispatch(1, 1, 1);
}