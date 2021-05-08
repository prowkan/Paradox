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

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *SceneLuminanceTextureUAVs[4];

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *SceneLuminanceTextureSRVs[4];

	for (int i = 0; i < 4; i++)
	{
		ResourceDesc.Height = Heights[i];
		ResourceDesc.Width = Widths[i];

		SceneLuminanceTextures[i] = renderGraph->CreateResource(ResourceDesc, String("SceneLuminanceTextures") + String(i));

		SceneLuminanceTextureUAVs[i] = SceneLuminanceTextures[i]->CreateView(UAVDesc, String("SceneLuminanceTextureUAVs") + String(i));

		SceneLuminanceTextureSRVs[i] = SceneLuminanceTextures[i]->CreateView(SRVDesc, String("SceneLuminanceTextureSRVs") + String(i));
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

	RenderGraphResource *AverageLuminanceTexture = renderGraph->CreateResource(ResourceDesc, "AverageLuminanceTexture");

	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *AverageLuminanceTextureUAV = AverageLuminanceTexture->CreateView(UAVDesc, "AverageLuminanceTextureUAV");
	
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	RenderGraphResourceView *AverageLuminanceTextureSRV = AverageLuminanceTexture->CreateView(SRVDesc, "AverageLuminanceTextureSRV");

	SceneLuminancePasses = new ComputePass*[5];

	SceneLuminancePasses[0] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 0");
	SceneLuminancePasses[1] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 1");
	SceneLuminancePasses[2] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 2");
	SceneLuminancePasses[3] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 3");
	SceneLuminancePasses[4] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 4");

	SceneLuminancePasses[0]->AddShaderResource(renderGraph->GetResource("ResolvedHDRSceneColorTexture")->GetView("ResolvedHDRSceneColorTextureSRV"));
	SceneLuminancePasses[0]->AddUnorderedAccess(SceneLuminanceTextureUAVs[0]);

	for (int i = 1; i < 4; i++)
	{
		SceneLuminancePasses[i]->AddShaderResource(SceneLuminanceTextureSRVs[i - 1]);
		SceneLuminancePasses[i]->AddUnorderedAccess(SceneLuminanceTextureUAVs[i]);
	}

	SceneLuminancePasses[4]->AddShaderResource(SceneLuminanceTextureSRVs[3]);
	SceneLuminancePasses[4]->AddUnorderedAccess(AverageLuminanceTextureUAV);

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