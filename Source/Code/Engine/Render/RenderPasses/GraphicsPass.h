#pragma once

#include "../RenderPass.h"

class GraphicsPass : public RenderPass
{
	public:

		void SetRenderTarget();

	private:
	
		VkRenderPass RenderPass;
		VkFramebuffer FrameBuffer;
};