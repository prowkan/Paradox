#pragma once

class RenderGraph;

class RenderStage
{
	public:

		virtual void Init(RenderGraph* renderGraph) = 0;
		virtual void Execute() = 0;

		virtual const char* GetName() = 0;
};