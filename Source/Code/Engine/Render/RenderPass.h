#pragma once

class RenderSystem;

class RenderPass
{
	public:

		virtual void Init(RenderSystem& renderSystem) = 0;
		virtual void Execute(RenderSystem& renderSystem) = 0;

		virtual const char* GetName() = 0;

	private:
};