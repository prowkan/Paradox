// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SkyAndFogStage.h"

#include "../RenderGraph.h"

void SkyAndFogStage::Init(RenderGraph* renderGraph)
{
	FogPass = renderGraph->CreateRenderPass<FullScreenPass>("Fog Pass");
	SkyPass = renderGraph->CreateRenderPass<GraphicsPass>("Sky Pass");

	FogPass->AddInput(renderGraph->GetResource("DepthBufferTexture")->GetView("DepthBufferTextureSRV"));
	FogPass->SetRenderTarget(renderGraph->GetResource("HDRSceneColorTexture")->GetView("HDRSceneColorTextureRTV"), 0);
	SkyPass->SetRenderTarget(renderGraph->GetResource("HDRSceneColorTexture")->GetView("HDRSceneColorTextureRTV"), 0);
	SkyPass->SetDepthStencil(renderGraph->GetResource("DepthBufferTexture")->GetView("DepthBufferTextureDSV"));

	FogPass->SetExecutionCallBack([=] () -> void 
	{
		//cout << "FogPass callback was called." << endl;
	});

	SkyPass->SetExecutionCallBack([=] () -> void
	{
		//cout << "SkyPass callback was called." << endl;
	});
}

void SkyAndFogStage::Execute()
{
	FogPass->ExecutePass();
	SkyPass->ExecutePass();
}