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

		D3D12_RESOURCE_DESC ResourceDesc;

		ResourceType Type;

		String Name;
};

class RenderGraph
{
	public:

		RenderGraphResource* CreateResource(D3D12_RESOURCE_DESC& ResourceDesc, const String& Name);

		RenderGraphResource* GetResource(const String& Name);

		template<typename T>
		T* CreateRenderPass(const String& Name);

		void ExportGraphToHTML();

	private:

		DynamicArray<RenderPass*> RenderPasses;
		DynamicArray<RenderGraphResource*> RenderResources;
};

template<typename T>
inline T* RenderGraph::CreateRenderPass(const String& Name)
{
	T* renderPass = new T();
	renderPass->Name = Name;
	RenderPasses.Add(renderPass);
	return renderPass;
}