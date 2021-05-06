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
	CascadedShadowMapTextures[0] = renderGraph->CreateResource(ResourceDesc, "CascadedShadowMapTexture0");
	CascadedShadowMapTextures[1] = renderGraph->CreateResource(ResourceDesc, "CascadedShadowMapTexture1");
	CascadedShadowMapTextures[2] = renderGraph->CreateResource(ResourceDesc, "CascadedShadowMapTexture2");
	CascadedShadowMapTextures[3] = renderGraph->CreateResource(ResourceDesc, "CascadedShadowMapTexture3");

	CascadedShadowMapPasses[0] = renderGraph->CreateRenderPass<ScenePass>("Cascaded Shadow Map Pass 0");
	CascadedShadowMapPasses[1] = renderGraph->CreateRenderPass<ScenePass>("Cascaded Shadow Map Pass 1");
	CascadedShadowMapPasses[2] = renderGraph->CreateRenderPass<ScenePass>("Cascaded Shadow Map Pass 2");
	CascadedShadowMapPasses[3] = renderGraph->CreateRenderPass<ScenePass>("Cascaded Shadow Map Pass 3");

	CascadedShadowMapPasses[0]->AddOutput(CascadedShadowMapTextures[0]);
	CascadedShadowMapPasses[1]->AddOutput(CascadedShadowMapTextures[1]);
	CascadedShadowMapPasses[2]->AddOutput(CascadedShadowMapTextures[2]);
	CascadedShadowMapPasses[3]->AddOutput(CascadedShadowMapTextures[3]);

	for (int i = 0; i < 4; i++)
	{
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