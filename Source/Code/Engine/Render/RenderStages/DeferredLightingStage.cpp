// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "DeferredLightingStage.h"

#include "../RenderGraph.h"

void DeferredLightingStage::Init(RenderGraph* renderGraph)
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
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	RenderGraphResource *HDRSceneColorTexture = renderGraph->CreateResource(ResourceDesc, "HDRSceneColorTexture");

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	RenderGraphResourceView *HDRSceneColorTextureRTV = HDRSceneColorTexture->CreateView(RTVDesc, "HDRSceneColorTextureRTV");

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	RenderGraphResourceView *HDRSceneColorTextureSRV = HDRSceneColorTexture->CreateView(SRVDesc, "HDRSceneColorTextureSRV");

	DeferredLightingPass = renderGraph->CreateRenderPass<FullScreenPass>("Deferred Lighting Pass");

	DeferredLightingPass->AddInput(renderGraph->GetResource("GBufferTexture0")->GetView("GBufferTexture0SRV"));
	DeferredLightingPass->AddInput(renderGraph->GetResource("GBufferTexture1")->GetView("GBufferTexture1SRV"));
	DeferredLightingPass->AddInput(renderGraph->GetResource("DepthBufferTexture")->GetView("DepthBufferTextureSRV"));
	DeferredLightingPass->AddInput(renderGraph->GetResource("ShadowMaskTexture")->GetView("ShadowMaskTextureSRV"));
	DeferredLightingPass->SetRenderTarget(HDRSceneColorTextureRTV, 0);

	DeferredLightingPass->SetExecutionCallBack([=] () -> void 
	{
		//cout << "DeferredLightingPass callback was called." << endl;
	});
}

void DeferredLightingStage::Execute()
{
	DeferredLightingPass->ExecutePass();
}