// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "HDRSceneColorResolveStage.h"

#include "../RenderGraph.h"

void HDRSceneColorResolveStage::Init(RenderGraph* renderGraph)
{
	uint32_t ResolutionWidth = 1280;
	uint32_t ResolutionHeight = 720;

	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.extent.height = ResolutionHeight;
	ImageCreateInfo.extent.width = ResolutionWidth;
	ImageCreateInfo.flags = 0;
	ImageCreateInfo.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
	ImageCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	ImageCreateInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.pNext = nullptr;
	ImageCreateInfo.pQueueFamilyIndices = nullptr;
	ImageCreateInfo.queueFamilyIndexCount = 0;
	ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

	RenderGraphResource *ResolvedHDRSceneColorTexture = renderGraph->CreateTexture(ImageCreateInfo, "ResolvedHDRSceneColorTexture");

	HDRSceneColorResolvePass = renderGraph->CreateRenderPass<ResolvePass>("HDR Scene Color Resolve Pass");
	
	HDRSceneColorResolvePass->AddInput(renderGraph->GetResource("HDRSceneColorTexture"));
	HDRSceneColorResolvePass->AddOutput(ResolvedHDRSceneColorTexture);
}

void HDRSceneColorResolveStage::Execute()
{

}