// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SkyAndFogStage.h"

#include "../RenderGraph.h"

void SkyAndFogStage::Init(RenderGraph* renderGraph)
{
	FogPass = renderGraph->CreateRenderPass<FullScreenPass>("Fog Pass");
	SkyPass = renderGraph->CreateRenderPass<RenderPass>("Sky Pass");

	FogPass->AddShaderResource(renderGraph->GetResource("DepthBufferTexture")->GetView("DepthBufferTextureSRV"));
	FogPass->AddRenderTarget(renderGraph->GetResource("HDRSceneColorTexture")->GetView("HDRSceneColorTextureRTV"));
	SkyPass->AddRenderTarget(renderGraph->GetResource("HDRSceneColorTexture")->GetView("HDRSceneColorTextureRTV"));
	SkyPass->AddDepthStencil(renderGraph->GetResource("DepthBufferTexture")->GetView("DepthBufferTextureDSV"));

	FogPass->SetExecutionCallBack([=] () -> void 
	{
		cout << "FogPass callback was called." << endl;
	});

	SkyPass->SetExecutionCallBack([=] () -> void
	{
		cout << "SkyPass callback was called." << endl;
	});
}

void SkyAndFogStage::Execute()
{
	FogPass->ExecutePass();
	SkyPass->ExecutePass();
}