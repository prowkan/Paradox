// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "ShadowMapStage.h"

#include "../RenderGraph.h"

void ShadowMapStage::Init(RenderGraph* renderGraph)
{
	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.extent.height = 2048;
	ImageCreateInfo.extent.width = 2048;
	ImageCreateInfo.flags = 0;
	ImageCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT;
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
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

	/*SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[0]));
	SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[1]));
	SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[2]));
	SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[3]));*/

	renderGraph->CreateTexture(ImageCreateInfo, "CascadedShadowMapTexture0");
	renderGraph->CreateTexture(ImageCreateInfo, "CascadedShadowMapTexture1");
	renderGraph->CreateTexture(ImageCreateInfo, "CascadedShadowMapTexture2");
	renderGraph->CreateTexture(ImageCreateInfo, "CascadedShadowMapTexture3");
}

void ShadowMapStage::Execute()
{
	
}