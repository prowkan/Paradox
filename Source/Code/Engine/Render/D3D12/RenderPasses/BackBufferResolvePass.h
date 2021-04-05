#pragma once

#include "../RenderPass.h"

#include "../RenderDeviceD3D12.h"

#include <Containers/COMRCPtr.h>

class BackBufferResolvePass : public RenderPass
{
	public:

		virtual void Init(RenderDeviceD3D12& renderDevice) override;
		virtual void Execute(RenderDeviceD3D12& renderDevice) override;

		virtual const char* GetName() override { return "BackBufferResolvePass"; }

	private:

		Texture *ToneMappedImageTexture;
};