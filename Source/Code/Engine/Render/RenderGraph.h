#pragma once

#include <Containers/String.h>
#include <Containers/DynamicArray.h>

class RenderPass;
class RenderGraphResourceView;

class RenderGraphResource
{
	friend class RenderGraph;

	public:

		const String& GetName() { return Name; }

		RenderGraphResourceView* CreateView(const D3D12_SHADER_RESOURCE_VIEW_DESC& ViewDesc, const String& Name);
		RenderGraphResourceView* CreateView(const D3D12_RENDER_TARGET_VIEW_DESC& ViewDesc, const String& Name);
		RenderGraphResourceView* CreateView(const D3D12_DEPTH_STENCIL_VIEW_DESC& ViewDesc, const String& Name);
		RenderGraphResourceView* CreateView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& ViewDesc, const String& Name);

		RenderGraphResourceView* GetView(const String& Name);

	private:
		
		enum class ResourceType { Buffer, Texture1D, Texture2D, Texture3D };

		D3D12_RESOURCE_DESC ResourceDesc;

		ResourceType Type;

		String Name;


		DynamicArray<RenderGraphResourceView*> Views;
};

class RenderGraphResourceView
{
	friend class RenderGraph;
	friend class RenderGraphResource;

	public:

		const String& GetName() { return Name; }

	private:

		RenderGraphResource *Resource;

		enum class ViewType { ShaderResource, RenderTarget, DepthStencil, UnorderedAccess };

		struct 
		{
			union
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
				D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
				D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
				D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			};
		} ViewDesc;

		ViewType Type;

		String Name;
};

class RenderGraph
{
	public:

		RenderGraphResource* CreateResource(D3D12_RESOURCE_DESC& ResourceDesc, const String& Name);

		RenderGraphResource* GetResource(const String& Name);

		template<typename T>
		T* CreateRenderPass(const String& Name);

		void CompileGraph();

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