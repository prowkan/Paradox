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

	for (int i = 0; i < 7; i++)
	{
		ResourceDesc.Height = ResolutionHeight >> i;
		ResourceDesc.Width = ResolutionWidth >> i;

		BloomTextures[0][i] = renderGraph->CreateResource(ResourceDesc, String("BloomTextures") + String(0) + String(i));
		BloomTextures[1][i] = renderGraph->CreateResource(ResourceDesc, String("BloomTextures") + String(1) + String(i));
		BloomTextures[2][i] = renderGraph->CreateResource(ResourceDesc, String("BloomTextures") + String(2) + String(i));
	}

	BloomPasses = new FullScreenPass*[3 * 7 + 6];

	for (int i = 0; i < 3 * 7 + 6; i++)
	{
		BloomPasses[i] = renderGraph->CreateRenderPass<FullScreenPass>(String("Post-Process Bloom Pass ") + String(i));
	}

	BloomPasses[0]->AddInput(renderGraph->GetResource("ResolvedHDRSceneColorTexture"));
	BloomPasses[0]->AddInput(renderGraph->GetResource("SceneLuminanceTexture0"));
	BloomPasses[0]->AddOutput(BloomTextures[0][0]);

	BloomPasses[1]->AddInput(BloomTextures[0][0]);
	BloomPasses[1]->AddOutput(BloomTextures[1][0]);

	BloomPasses[2]->AddInput(BloomTextures[1][0]);
	BloomPasses[2]->AddOutput(BloomTextures[2][0]);

	int Index = 3;

	for (int i = 0; i < 6; i++)
	{
		BloomPasses[Index]->AddInput(BloomTextures[0][i]);
		BloomPasses[Index]->AddOutput(BloomTextures[0][i + 1]);

		Index++;

		BloomPasses[Index]->AddInput(BloomTextures[0][i + 1]);
		BloomPasses[Index]->AddOutput(BloomTextures[1][i + 1]);

		Index++;

		BloomPasses[Index]->AddInput(BloomTextures[1][i + 1]);
		BloomPasses[Index]->AddOutput(BloomTextures[2][i + 1]);

		Index++;
	}

	for (int i = 5; i >= 0; i--)
	{
		BloomPasses[Index]->AddInput(BloomTextures[2][i + 1]);
		BloomPasses[Index]->AddOutput(BloomTextures[2][i]);

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