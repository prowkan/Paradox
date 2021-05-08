// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GBufferOpaqueStage.h"

#include "../RenderGraph.h"

void GBufferOpaqueStage::Init(RenderGraph* renderGraph)
{
	uint32_t ResolutionWidth = 1280;
	uint32_t ResolutionHeight = 720;

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RenderGraphResource *GBufferTexture0 = renderGraph->CreateResource(ResourceDesc, "GBufferTexture0");
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	RenderGraphResource *GBufferTexture1 = renderGraph->CreateResource(ResourceDesc, "GBufferTexture1");

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RenderGraphResourceView *GBufferTexture0RTV = GBufferTexture0->CreateView(RTVDesc, "GBufferTexture0RTV");

	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	RenderGraphResourceView *GBufferTexture1RTV = GBufferTexture1->CreateView(RTVDesc, "GBufferTexture1RTV");

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RenderGraphResourceView *GBufferTexture0SRV = GBufferTexture0->CreateView(SRVDesc, "GBufferTexture0SRV");

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
	RenderGraphResourceView *GBufferTexture1SRV = GBufferTexture1->CreateView(SRVDesc, "GBufferTexture1SRV");

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	RenderGraphResource *DepthBufferTexture = renderGraph->CreateResource(ResourceDesc, "DepthBufferTexture");

	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2DMS;

	RenderGraphResourceView *DepthBufferTextureDSV = DepthBufferTexture->CreateView(DSVDesc, "DepthBufferTextureDSV");

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	RenderGraphResourceView *DepthBufferTextureSRV = DepthBufferTexture->CreateView(SRVDesc, "DepthBufferTextureSRV");

	GBufferOpaquePass = renderGraph->CreateRenderPass<ScenePass>("G-Buffer Opaque Pass");

	GBufferOpaquePass->AddRenderTarget(GBufferTexture0RTV);
	GBufferOpaquePass->AddRenderTarget(GBufferTexture1RTV);
	GBufferOpaquePass->AddDepthStencil(DepthBufferTextureDSV);

	GBufferOpaquePass->SetExecutionCallBack([=] () -> void
	{
		cout << "GBufferOpaquePass callback was called." << endl;
	});
}

void GBufferOpaqueStage::Execute()
{
	GBufferOpaquePass->ExecutePass();
}