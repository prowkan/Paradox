#pragma once

class RenderDeviceD3D12;

class RenderPass
{
	public:

		virtual void Init(RenderDeviceD3D12& renderDevice) = 0;
		virtual void Execute(RenderDeviceD3D12& renderDevice) = 0;

		virtual const char* GetName() = 0;

	private:
};