#pragma once

class RenderPass
{
	public:

		virtual void Init() = 0;
		virtual void Execute() = 0;

	private:
};