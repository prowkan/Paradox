#pragma once

#include <Containers/String.h>
#include <Containers/DynamicArray.h>

class RenderPass;

class RenderGraphResource
{
	friend class RenderGraph;

	public:

		const String& GetName() { return Name; }

	private:
		
		enum class ResourceType { Buffer, Texture1D, Texture2D, Texture3D };

		union
		{
			VkImageCreateInfo ImageCreateInfo;
			VkBufferCreateInfo BufferCreateInfo;
		};

		ResourceType Type;

		String Name;
};

class RenderGraph
{
	public:

		RenderGraphResource* CreateBuffer(VkBufferCreateInfo& BufferCreateInfo, const String& Name);
		RenderGraphResource* CreateTexture(VkImageCreateInfo& ImageCreateInfo, const String& Name);

		RenderGraphResource* GetResource(const String& Name);

	private:

		DynamicArray<RenderPass*> RenderPasses;
		DynamicArray<RenderGraphResource*> RenderResources;
};