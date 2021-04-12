#pragma once

class RenderDevice;

class RenderStage
{
	public:

		virtual void Init(RenderDevice* renderDevice) = 0;
		virtual void Execute(RenderDevice* renderDevice) = 0;

		virtual const char* GetName() = 0;
};