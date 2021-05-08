// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ShadowMapStage.h"

#include "../RenderGraph.h"

void ShadowMapStage::Init(RenderGraph* renderGraph)
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	ResourceDesc.Height = 2048;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 2048;

	RenderGraphResource *CascadedShadowMapTextures[4];

	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	DSVDesc.Texture2D.MipSlice = 0;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *CascadedShadowMapTextureDSVs[4];

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *CascadedShadowMapTextureSRVs[4];

	for (int i = 0; i < 4; i++)
	{
		CascadedShadowMapTextures[i] = renderGraph->CreateResource(ResourceDesc, "CascadedShadowMapTexture" + i);
		CascadedShadowMapTextureDSVs[i] = CascadedShadowMapTextures[i]->CreateView(DSVDesc, "CascadedShadowMapTextureDSV" + i);
		CascadedShadowMapTextureSRVs[i] = CascadedShadowMapTextures[i]->CreateView(SRVDesc, "CascadedShadowMapTextureSRV" + i);

		CascadedShadowMapPasses[i] = renderGraph->CreateRenderPass<ScenePass>("Cascaded Shadow Map Pass " + i);

		CascadedShadowMapPasses[i]->AddOutput(CascadedShadowMapTextures[i]);

		CascadedShadowMapPasses[i]->SetExecutionCallBack([=] () -> void
		{
			cout << "CascadedShadowMapPass[" << i << "] callback was called." << endl;
		});
	}
}

void ShadowMapStage::Execute()
{
	for (int i = 0; i < 4; i++)
	{
		CascadedShadowMapPasses[i]->ExecutePass();
	}
}