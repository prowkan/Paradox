// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "SkyAndFogStage.h"

#include "../RenderGraph.h"

void SkyAndFogStage::Init(RenderGraph* renderGraph)
{
	FogPass = renderGraph->CreateRenderPass<FullScreenPass>("Fog Pass");
	SkyPass = renderGraph->CreateRenderPass<RenderPass>("Sky Pass");
}

void SkyAndFogStage::Execute()
{
	
}