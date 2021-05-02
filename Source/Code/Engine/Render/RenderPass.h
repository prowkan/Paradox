#pragma once

#include <Containers/String.h>
#include <Containers/DynamicArray.h>

class RenderGraphResource;

class RenderPass
{
	friend class RenderGraph;

	public:

		void AddInput(RenderGraphResource* Resource)
		{
			RenderPassInputs.Add(Resource);
		}

		void AddOutput(RenderGraphResource* Resource)
		{
			RenderPassOutputs.Add(Resource);
		}

	private:

		String Name;

		DynamicArray<RenderGraphResource*> RenderPassInputs;
		DynamicArray<RenderGraphResource*> RenderPassOutputs;
};