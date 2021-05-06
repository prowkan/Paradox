// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessLuminanceStage.h"

#include "../RenderGraph.h"

void PostProcessLuminanceStage::Init(RenderGraph* renderGraph)
{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;

	int Widths[4] = { 1280, 80, 5, 1 };
	int Heights[4] = { 720, 45, 3, 1 };

	RenderGraphResource *SceneLuminanceTextures[4];

	for (int i = 0; i < 4; i++)
	{
		ResourceDesc.Height = Heights[i];
		ResourceDesc.Width = Widths[i];

		SceneLuminanceTextures[i] = renderGraph->CreateResource(ResourceDesc, String("SceneLuminanceTexture") + String(i));
	}

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ResourceDesc.Height = 1;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 1;

	//SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &AverageLuminanceTexture));
	RenderGraphResource *AverageLuminanceTexture = renderGraph->CreateResource(ResourceDesc, "AverageLuminanceTexture");

	SceneLuminancePasses = new ComputePass*[5];

	SceneLuminancePasses[0] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 0");
	SceneLuminancePasses[1] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 1");
	SceneLuminancePasses[2] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 2");
	SceneLuminancePasses[3] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 3");
	SceneLuminancePasses[4] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 4");

	SceneLuminancePasses[0]->AddInput(renderGraph->GetResource("ResolvedHDRSceneColorTexture"));
	SceneLuminancePasses[0]->AddOutput(SceneLuminanceTextures[0]);

	for (int i = 1; i < 4; i++)
	{
		SceneLuminancePasses[i]->AddInput(SceneLuminanceTextures[i - 1]);
		SceneLuminancePasses[i]->AddOutput(SceneLuminanceTextures[i]);
	}

	SceneLuminancePasses[4]->AddInput(SceneLuminanceTextures[3]);
	SceneLuminancePasses[4]->AddOutput(AverageLuminanceTexture);

	for (int i = 0; i < 5; i++)
	{
		SceneLuminancePasses[i]->SetExecutionCallBack([=] () -> void
		{
			cout << "SceneLuminancePass[" << i << "] callback was called." << endl;
		});
	}
}

void PostProcessLuminanceStage::Execute()
{
	for (int i = 0; i < 5; i++)
	{
		SceneLuminancePasses[i]->ExecutePass();
	}
}