// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessHDRToneMappingStage.h"

#include "../RenderGraph.h"

void PostProcessHDRToneMappingStage::Init(RenderGraph* renderGraph)
{
	uint32_t ResolutionWidth = 1280;
	uint32_t ResolutionHeight = 720;

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	RenderGraphResource *ToneMappedImageTexture = renderGraph->CreateResource(ResourceDesc, "ToneMappedImageTexture");

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	RenderGraphResourceView *ToneMappedImageTextureRTV = ToneMappedImageTexture->CreateView(RTVDesc, "ToneMappedImageTextureRTV");
	
	PostProcessHDRToneMappingPass = renderGraph->CreateRenderPass<FullScreenPass>("Post-Process HDR Tone Mapping Pass");

	PostProcessHDRToneMappingPass->AddInput(renderGraph->GetResource("HDRSceneColorTexture"));
	PostProcessHDRToneMappingPass->AddInput(renderGraph->GetResource("BloomTextures20"));
	PostProcessHDRToneMappingPass->AddOutput(ToneMappedImageTexture);

	PostProcessHDRToneMappingPass->SetExecutionCallBack([=] () -> void 
	{
		cout << "PostProcessHDRToneMappingPass callback was called." << endl;
	});
}

void PostProcessHDRToneMappingStage::Execute()
{
	PostProcessHDRToneMappingPass->ExecutePass();
}