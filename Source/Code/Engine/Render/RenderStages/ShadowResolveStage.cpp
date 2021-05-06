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

	ShadowResolvePass = renderGraph->CreateRenderPass<FullScreenPass>("Shadow Resolve Pass");

	ShadowResolvePass->AddInput(renderGraph->GetResource("ResolvedDepthBufferTexture"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTexture0"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTexture1"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTexture2"));
	ShadowResolvePass->AddInput(renderGraph->GetResource("CascadedShadowMapTexture3"));
	ShadowResolvePass->AddOutput(ShadowMaskTexture);

	ShadowResolvePass->SetExecutionCallBack([=] () -> void
	{
		cout << "ShadowResolvePass callback was called." << endl;
	});
}

void ShadowResolveStage::Execute()
{
	ShadowResolvePass->ExecutePass();
}