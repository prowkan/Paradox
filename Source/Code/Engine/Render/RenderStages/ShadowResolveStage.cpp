// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ShadowResolveStage.h"

#include "../RenderGraph.h"

void ShadowResolveStage::Init(RenderGraph* renderGraph)
{
	uint32_t ResolutionWidth = 1280;
	uint32_t ResolutionHeight = 720;

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	RenderGraphResource *ShadowMaskTexture = renderGraph->CreateResource(ResourceDesc, "ShadowMaskTexture");

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *ShadowMaskTextureRTV = ShadowMaskTexture->CreateView(RTVDesc, "ShadowMaskTextureRTV");

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *ShadowMaskTextureSRV = ShadowMaskTexture->CreateView(SRVDesc, "ShadowMaskTextureSRV");

	ShadowResolvePass = renderGraph->CreateRenderPass<FullScreenPass>("Shadow Resolve Pass");

	ShadowResolvePass->AddInput(renderGraph->GetResource("ResolvedDepthBufferTexture")->GetView("ResolvedDepthBufferTextureSRV"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTextures0")->GetView("CascadedShadowMapTextureSRVs0"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTextures1")->GetView("CascadedShadowMapTextureSRVs1"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTextures2")->GetView("CascadedShadowMapTextureSRVs2"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTextures3")->GetView("CascadedShadowMapTextureSRVs3"));
	ShadowResolvePass->SetRenderTarget(ShadowMaskTextureRTV, 0);

	ShadowResolvePass->SetExecutionCallBack([=] () -> void
	{
		cout << "ShadowResolvePass callback was called." << endl;
	});
}

void ShadowResolveStage::Execute()
{
	ShadowResolvePass->ExecutePass();
}