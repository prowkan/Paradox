// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "HDRSceneColorResolveStage.h"

#include "../RenderGraph.h"

void HDRSceneColorResolveStage::Init(RenderGraph* renderGraph)
{
	uint32_t ResolutionWidth = 1280;
	uint32_t ResolutionHeight = 720;

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	RenderGraphResource *ResolvedHDRSceneColorTexture = renderGraph->CreateResource(ResourceDesc, "ResolvedHDRSceneColorTexture");

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *ResolvedHDRSceneColorTextureView = ResolvedHDRSceneColorTexture->CreateView(SRVDesc, "ResolvedHDRSceneColorTextureSRV");

	HDRSceneColorResolvePass = renderGraph->CreateRenderPass<ResolvePass>("HDR Scene Color Resolve Pass");
	
	HDRSceneColorResolvePass->AddInput(renderGraph->GetResource("HDRSceneColorTexture"));
	HDRSceneColorResolvePass->AddOutput(ResolvedHDRSceneColorTexture);
}

void HDRSceneColorResolveStage::Execute()
{

}