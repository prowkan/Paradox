// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderGraph.h"

#include "RenderPass.h"
#include "RenderPasses/ResolvePass.h"

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

void RenderGraph::CompileGraph()
{
    auto BeginTime = chrono::high_resolution_clock::now();

    size_t RenderPassesCount = RenderPasses.GetLength();

    for (RenderGraphResource* Resource : RenderResources)
    {
        cout << Resource->GetName().GetData() << endl;

        for (size_t i = RenderPassesCount - 1; (int64_t)i >= 0; i--)
        {
            if (RenderPasses[i]->IsResourceUsedInRenderPass(Resource))
            {
                RenderPass::ResourceUsageType DestResourceUsageType = RenderPasses[i]->GetResourceUsageType(Resource);

                int64_t j = i - 1;
                if (j == -1) j = RenderPassesCount - 1;

                while (true)
                {
                    if (RenderPasses[j]->IsResourceUsedInRenderPass(Resource))
                    {
                        RenderPass::ResourceUsageType SourceResourceUsageType = RenderPasses[j]->GetResourceUsageType(Resource);

                        if (SourceResourceUsageType != DestResourceUsageType)
                        {
                            D3D12_RESOURCE_STATES OldState, NewState;

                            switch (SourceResourceUsageType)
                            {
                                case RenderPass::ResourceUsageType::ShaderResource:
                                    OldState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                                    break;
                                case RenderPass::ResourceUsageType::RenderTarget:
                                    OldState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
                                    break;
                                case RenderPass::ResourceUsageType::DepthStencil:
                                    OldState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
                                    break;
                                case RenderPass::ResourceUsageType::UnorderedAccess:
                                    OldState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                                    break;
                                case RenderPass::ResourceUsageType::ResolveInput:
                                    OldState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
                                    break;
                                case RenderPass::ResourceUsageType::ResolveOutput:
                                    OldState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
                                    break;
                            }

                            switch (DestResourceUsageType)
                            {
                                case RenderPass::ResourceUsageType::ShaderResource:
                                    NewState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                                    break;
                                case RenderPass::ResourceUsageType::RenderTarget:
                                    NewState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
                                    break;
                                case RenderPass::ResourceUsageType::DepthStencil:
                                    NewState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
                                    break;
                                case RenderPass::ResourceUsageType::UnorderedAccess:
                                    NewState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                                    break;
                                case RenderPass::ResourceUsageType::ResolveInput:
                                    NewState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
                                    break;
                                case RenderPass::ResourceUsageType::ResolveOutput:
                                    NewState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
                                    break;
                            }

                            RenderPasses[i]->ResourceBarriers.Add(RenderPass::ResourceBarrier{ Resource, 0, OldState, NewState });
                        }

                        break;
                    }

                    j--;

                    if (j == -1) j = RenderPassesCount - 1;
                    if (j == i) break;
                }                
            }
        }
    }

    auto EndTime = chrono::high_resolution_clock::now();

    float GraphCompilationTime = float(chrono::duration_cast<chrono::milliseconds>(EndTime - BeginTime).count());

    cout << "Graph Compilation Time: " << GraphCompilationTime << " ms." << endl;
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

            if (renderPass->IsResourceUsedInRenderPass(renderGraphResource)) { RenderPassBeginAccessIndex = i; break; }            
        }

        for (size_t i = RenderPassesCount - 1; (int64_t)i >= 0; i--)
        {
            RenderPass* renderPass = RenderPasses[i];

            if (renderPass->IsResourceUsedInRenderPass(renderGraphResource)) { RenderPassEndAccessIndex = i; break; }
        }

        OutputHTMLFile << "\t\t\t<TR>\n";

        OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">";
        OutputHTMLFile << renderGraphResource->Name.GetData();
        OutputHTMLFile << "</TD>\n";

        for (size_t i = 0; i < RenderPassesCount; i++)
        {
            if (i >= RenderPassBeginAccessIndex && i <= RenderPassEndAccessIndex)
            {
                RenderPass* renderPass = RenderPasses[i];

                bool IsResourceRead = false;
                bool IsResourceWritten = false;

                IsResourceRead = renderPass->IsResourceReadInRenderPass(renderGraphResource);
                IsResourceWritten = renderPass->IsResourceWrittenInRenderPass(renderGraphResource);

                String BarrierStr = "&nbsp;";

                for (RenderPass::ResourceBarrier& ResourceBarrier : renderPass->ResourceBarriers)
                {
                    if (ResourceBarrier.Resource == renderGraphResource)
                    {
                        BarrierStr = "";

                        if (ResourceBarrier.OldState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET) BarrierStr += "RT";
                        if (ResourceBarrier.OldState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE) BarrierStr += "DS";
                        if (ResourceBarrier.OldState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS) BarrierStr += "UA";
                        if (ResourceBarrier.OldState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE) BarrierStr += "RS";
                        if (ResourceBarrier.OldState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST) BarrierStr += "RD";
                        if ((ResourceBarrier.OldState == (D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) || (ResourceBarrier.OldState == (D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ))) BarrierStr += "SR";
                    
                        BarrierStr += "&nbsp;->&nbsp;";

                        if (ResourceBarrier.NewState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET) BarrierStr += "RT";
                        if (ResourceBarrier.NewState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE) BarrierStr += "DS";
                        if (ResourceBarrier.NewState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS) BarrierStr += "UA";
                        if (ResourceBarrier.NewState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE) BarrierStr += "RS";
                        if (ResourceBarrier.NewState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST) BarrierStr += "RD";
                        if ((ResourceBarrier.NewState == (D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) || (ResourceBarrier.NewState == (D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ))) BarrierStr += "SR";
                    }
                }

                if (IsResourceRead)
                    OutputHTMLFile << "\t\t\t\t<TD style=\"border-top: 1px solid black; border-bottom: 1px solid black; background-color: #00C000\">" << BarrierStr.GetData() << "</TD>\n";
                else if (IsResourceWritten)
                    OutputHTMLFile << "\t\t\t\t<TD style=\"border-top: 1px solid black; border-bottom: 1px solid black; background-color: #C00000\">" << BarrierStr.GetData() << "</TD>\n";
                else
                    OutputHTMLFile << "\t\t\t\t<TD style=\"border-top: 1px solid black; border-bottom: 1px solid black; background-color: #C0C0C0\">" << BarrierStr.GetData() << "</TD>\n";
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
