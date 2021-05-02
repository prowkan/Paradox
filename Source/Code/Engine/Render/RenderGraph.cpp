// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderGraph.h"

#include "RenderPass.h"

RenderGraphResource* RenderGraph::CreateBuffer(VkBufferCreateInfo& BufferCreateInfo, const String& Name)
{
    RenderGraphResource* Resource = new RenderGraphResource();

    Resource->Name = Name;

    Resource->BufferCreateInfo = BufferCreateInfo;

    Resource->Type = RenderGraphResource::ResourceType::Buffer;

    RenderResources.Add(Resource);

    return Resource;
}

RenderGraphResource* RenderGraph::CreateTexture(VkImageCreateInfo& ImageCreateInfo, const String& Name)
{
    RenderGraphResource* Resource = new RenderGraphResource();

    Resource->Name = Name;

    Resource->ImageCreateInfo = ImageCreateInfo;

    switch (ImageCreateInfo.imageType)
    {
        case VkImageType::VK_IMAGE_TYPE_1D:
            Resource->Type = RenderGraphResource::ResourceType::Texture1D;
            break;
        case VkImageType::VK_IMAGE_TYPE_2D:
            Resource->Type = RenderGraphResource::ResourceType::Texture2D;
            break;
        case VkImageType::VK_IMAGE_TYPE_3D:
            Resource->Type = RenderGraphResource::ResourceType::Texture3D;
            break;
    }    

    RenderResources.Add(Resource);

    return Resource;
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

    for (RenderPass* renderPass : RenderPasses)
    {
        OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">";
        OutputHTMLFile << renderPass->Name.GetData();
        OutputHTMLFile << "</TD>\n";
    }

    OutputHTMLFile << "\t\t\t</TR>\n";

    for (RenderGraphResource* renderGraphResource : RenderResources)
    {
        OutputHTMLFile << "\t\t\t<TR>\n";

        OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">";
        OutputHTMLFile << renderGraphResource->Name.GetData();
        OutputHTMLFile << "</TD>\n";

        for (RenderPass* renderPass : RenderPasses)
        {
            OutputHTMLFile << "\t\t\t\t<TD style=\"border: 1px solid black\">&nbsp;</TD>\n";
        }

        OutputHTMLFile << "\t\t\t</TR>\n";
    }

    OutputHTMLFile << "\t\t</TABLE>\n";
    OutputHTMLFile << "\t</BODY>\n";
    OutputHTMLFile << "</HTML>\n";
    OutputHTMLFile.close();
}
