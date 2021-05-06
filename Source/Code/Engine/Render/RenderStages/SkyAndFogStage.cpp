// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SkyAndFogStage.h"

#include "../RenderGraph.h"

void SkyAndFogStage::Init(RenderGraph* renderGraph)
{
	FogPass = renderGraph->CreateRenderPass<FullScreenPass>("Fog Pass");
	SkyPass = renderGraph->CreateRenderPass<RenderPass>("Sky Pass");

	FogPass->AddInput(renderGraph->GetResource("DepthBufferTexture"));
	FogPass->AddOutput(renderGraph->GetResource("HDRSceneColorTexture"));
	SkyPass->AddOutput(renderGraph->GetResource("HDRSceneColorTexture"));
	SkyPass->AddOutput(renderGraph->GetResource("DepthBufferTexture"));

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