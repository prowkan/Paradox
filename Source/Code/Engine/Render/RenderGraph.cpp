// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderGraph.h"

#include "RenderPass.h"

RenderGraphResource* RenderGraph::CreateResource(D3D12_RESOURCE_DESC& ResourceDesc, const String& Name)
{
    RenderGraphResource* Resource = new RenderGraphResource();

    Resource->Name = Name;

    Resource->ResourceDesc = ResourceDesc;

    switch (ResourceDesc.Dimension)
    {
        case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER:
            Resource->Type = RenderGraphResource::ResourceType::Buffer;
            break;
        case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            Resource->Type = RenderGraphResource::ResourceType::Texture1D;
            break;
        case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            Resource->Type = RenderGraphResource::ResourceType::Texture2D;
            break;
        case D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            Resource->Type = RenderGraphResource::ResourceType::Texture3D;
            break;
    }    

    RenderResources.Add(Resource);

    return Resource;
}

RenderGraphResourceView* RenderGraphResource::CreateView(const D3D12_SHADER_RESOURCE_VIEW_DESC& ViewDesc, const String& Name)
{
    RenderGraphResourceView *View = new RenderGraphResourceView();
    View->Name = Name;
    View->Resource = this;
    View->Type = RenderGraphResourceView::ViewType::ShaderResource;
    View->ViewDesc.SRVDesc = ViewDesc;

    Views.Add(View);

    return View;
}

RenderGraphResourceView* RenderGraphResource::CreateView(const D3D12_RENDER_TARGET_VIEW_DESC& ViewDesc, const String& Name)
{
    RenderGraphResourceView *View = new RenderGraphResourceView();
    View->Name = Name;
    View->Resource = this;
    View->Type = RenderGraphResourceView::ViewType::RenderTarget;
    View->ViewDesc.RTVDesc = ViewDesc;

    Views.Add(View);

    return View;
}

RenderGraphResourceView* RenderGraphResource::CreateView(const D3D12_DEPTH_STENCIL_VIEW_DESC& ViewDesc, const String& Name)
{
    RenderGraphResourceView *View = new RenderGraphResourceView();
    View->Name = Name;
    View->Resource = this;
    View->Type = RenderGraphResourceView::ViewType::DepthStencil;
    View->ViewDesc.DSVDesc = ViewDesc;

    Views.Add(View);

    return View;
}

RenderGraphResourceView* RenderGraphResource::CreateView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& ViewDesc, const String& Name)
{
    RenderGraphResourceView *View = new RenderGraphResourceView();
    View->Name = Name;
    View->Resource = this;
    View->Type = RenderGraphResourceView::ViewType::UnorderedAccess;
    View->ViewDesc.UAVDesc = ViewDesc;

    Views.Add(View);

    return View;
}

RenderGraphResourceView* RenderGraphResource::GetView(const String& Name)
{
    for (RenderGraphResourceView* View : Views)
    {
        if (View->GetName() == Name)
        {
            return View;
        }
    }

    return nullptr;
}

RenderGraphResource* RenderGraph::GetResource(const String& Name)
{
    for (RenderGraphResource* Resource : RenderResources)
    {
        if (Resource->GetName() == Name)
        {
            return Resource;
        }
    }

    return nullptr;
}

void RenderGraph::ExportGraphToHTML()
{
    fstream OutputHTMLFile("RenderGraph.html", ios::out);
    OutputHTMLFile << "<HTML>\n";
    OutputHTMLFile << "\t<BODY>\n";
    OutputHTMLFile << "\t\t<TABLE cellspacing=\"0\" cellpadding=\"0\" style=\"border-collapse: collapse\">\n";
    OutputHTMLFile << "\t\t\t<TR>\n";

    OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">&nbsp;</TD>\n";

    size_t RenderPassesCount = RenderPasses.GetLength();

    for (RenderPass* renderPass : RenderPasses)
    {
        OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">";
        OutputHTMLFile << renderPass->Name.GetData();
        OutputHTMLFile << "</TD>\n";
    }

    OutputHTMLFile << "\t\t\t</TR>\n";

    for (RenderGraphResource* renderGraphResource : RenderResources)
    {
        size_t RenderPassBeginAccessIndex = -1, RenderPassEndAccessIndex = -1;

        for (size_t i = 0; i < RenderPassesCount; i++)
        {
            RenderPass* renderPass = RenderPasses[i];

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassShaderResources)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassBeginAccessIndex = i; break; }
            }

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassRenderTargets)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassBeginAccessIndex = i; break; }
            }

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassDepthStencils)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassBeginAccessIndex = i; break; }
            }

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassUnorderedAccesses)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassBeginAccessIndex = i; break; }
            }

            if (RenderPassBeginAccessIndex == i) break;
        }

        for (size_t i = RenderPassesCount - 1; (int64_t)i >= 0; i--)
        {
            RenderPass* renderPass = RenderPasses[i];

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassShaderResources)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassEndAccessIndex = i; break; }
            }

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassRenderTargets)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassEndAccessIndex = i; break; }
            }

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassDepthStencils)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassEndAccessIndex = i; break; }
            }

            for (RenderGraphResourceView* resourceView : renderPass->RenderPassUnorderedAccesses)
            {
                if (resourceView->Resource == renderGraphResource) { RenderPassEndAccessIndex = i; break; }
            }

            if (RenderPassEndAccessIndex == i) break;
        }

        OutputHTMLFile << "\t\t\t<TR>\n";

        OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">";
        OutputHTMLFile << renderGraphResource->Name.GetData();
        OutputHTMLFile << "</TD>\n";

        for (size_t i = 0; i < RenderPassesCount; i++)
        {
            if (i >= RenderPassBeginAccessIndex && i <= RenderPassEndAccessIndex)
            {
                OutputHTMLFile << "\t\t\t\t<TD style=\"border-top: 1px solid black; border-bottom: 1px solid black; background-color: #C0C0C0\">&nbsp;</TD>\n";
            }
            else
            {
                OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">&nbsp;</TD>\n";
            }
        }

        OutputHTMLFile << "\t\t\t</TR>\n";
    }

    OutputHTMLFile << "\t\t</TABLE>\n";
    OutputHTMLFile << "\t</BODY>\n";
    OutputHTMLFile << "</HTML>\n";
    OutputHTMLFile.close();
}
