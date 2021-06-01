// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessBloomStage.h"

#include "../RenderGraph.h"

void PostProcessBloomStage::Init(RenderGraph* renderGraph)
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
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;

	RenderGraphResource *BloomTextures[3][7];

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *BloomTextureRTVs[3][7];

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *BloomTextureSRVs[3][7];

	for (int i = 0; i < 7; i++)
	{
		ResourceDesc.Height = ResolutionHeight >> i;
		ResourceDesc.Width = ResolutionWidth >> i;

		BloomTextures[0][i] = renderGraph->CreateResource(ResourceDesc, String("BloomTextures") + String(0) + String(i));
		BloomTextures[1][i] = renderGraph->CreateResource(ResourceDesc, String("BloomTextures") + String(1) + String(i));
		BloomTextures[2][i] = renderGraph->CreateResource(ResourceDesc, String("BloomTextures") + String(2) + String(i));

		BloomTextureRTVs[0][i] = BloomTextures[0][i]->CreateView(RTVDesc, String("BloomTextureRTVs") + String(0) + String(i));
		BloomTextureRTVs[1][i] = BloomTextures[1][i]->CreateView(RTVDesc, String("BloomTextureRTVs") + String(1) + String(i));
		BloomTextureRTVs[2][i] = BloomTextures[2][i]->CreateView(RTVDesc, String("BloomTextureRTVs") + String(2) + String(i));

		BloomTextureSRVs[0][i] = BloomTextures[0][i]->CreateView(SRVDesc, String("BloomTextureSRVs") + String(0) + String(i));
		BloomTextureSRVs[1][i] = BloomTextures[1][i]->CreateView(SRVDesc, String("BloomTextureSRVs") + String(1) + String(i));
		BloomTextureSRVs[2][i] = BloomTextures[2][i]->CreateView(SRVDesc, String("BloomTextureSRVs") + String(2) + String(i));
	}

	BloomPasses = new FullScreenPass*[3 * 7 + 6];

	for (int i = 0; i < 3 * 7 + 6; i++)
	{
		BloomPasses[i] = renderGraph->CreateRenderPass<FullScreenPass>(String("Post-Process Bloom Pass ") + String(i));
	}

	BloomPasses[0]->AddInput(renderGraph->GetResource("ResolvedHDRSceneColorTexture")->GetView("ResolvedHDRSceneColorTextureSRV"));
	BloomPasses[0]->AddInput(renderGraph->GetResource("SceneLuminanceTextures0")->GetView("SceneLuminanceTextureSRVs0"));
	BloomPasses[0]->SetRenderTarget(BloomTextureRTVs[0][0], 0);

	BloomPasses[1]->AddInput(BloomTextureSRVs[0][0]);
	BloomPasses[1]->SetRenderTarget(BloomTextureRTVs[1][0], 0);

	BloomPasses[2]->AddInput(BloomTextureSRVs[1][0]);
	BloomPasses[2]->SetRenderTarget(BloomTextureRTVs[2][0], 0);

	int Index = 3;

	for (int i = 0; i < 6; i++)
	{
		BloomPasses[Index]->AddInput(BloomTextureSRVs[0][i]);
		BloomPasses[Index]->SetRenderTarget(BloomTextureRTVs[0][i + 1], 0);

		Index++;

		BloomPasses[Index]->AddInput(BloomTextureSRVs[0][i + 1]);
		BloomPasses[Index]->SetRenderTarget(BloomTextureRTVs[1][i + 1], 0);

		Index++;

		BloomPasses[Index]->AddInput(BloomTextureSRVs[1][i + 1]);
		BloomPasses[Index]->SetRenderTarget(BloomTextureRTVs[2][i + 1], 0);

		Index++;
	}

	for (int i = 5; i >= 0; i--)
	{
		BloomPasses[Index]->AddInput(BloomTextureSRVs[2][i + 1]);
		BloomPasses[Index]->SetRenderTarget(BloomTextureRTVs[2][i], 0);

		Index++;
	}

	for (int i = 0; i < 3 * 7 + 6; i++)
	{
		BloomPasses[i]->SetExecutionCallBack([=] () -> void
		{
			cout << "BloomPass[" << i << "] callback was called." << endl;
		});
	}
}

void PostProcessBloomStage::Execute()
{
	for (int i = 0; i < 3 * 7 + 6; i++)
	{
		BloomPasses[i]->ExecutePass();
	}
}