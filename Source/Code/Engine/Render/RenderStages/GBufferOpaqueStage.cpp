// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "GBufferOpaqueStage.h"

#include "../RenderGraph.h"

void GBufferOpaqueStage::Init(RenderGraph* renderGraph)
{
	uint32_t ResolutionWidth = 1280;
	uint32_t ResolutionHeight = 720;

	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.extent.height = ResolutionHeight;
	ImageCreateInfo.extent.width = ResolutionWidth;
	ImageCreateInfo.flags = 0;
	//ImageCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
	ImageCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	ImageCreateInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.pNext = nullptr;
	ImageCreateInfo.pQueueFamilyIndices = nullptr;
	ImageCreateInfo.queueFamilyIndexCount = 0;
	ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
	ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

	ImageCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
	//SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &GBufferTextures[0]));
	RenderGraphResource *GBufferTexture0 = renderGraph->CreateTexture(ImageCreateInfo, "GBufferTexture0");
	ImageCreateInfo.format = VkFormat::VK_FORMAT_A2R10G10B10_UNORM_PACK32;
	//SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &GBufferTextures[1]));
	RenderGraphResource *GBufferTexture1 = renderGraph->CreateTexture(ImageCreateInfo, "GBufferTexture1");

	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.extent.height = ResolutionHeight;
	ImageCreateInfo.extent.width = ResolutionWidth;
	ImageCreateInfo.flags = 0;
	ImageCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
	ImageCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	ImageCreateInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.pNext = nullptr;
	ImageCreateInfo.pQueueFamilyIndices = nullptr;
	ImageCreateInfo.queueFamilyIndexCount = 0;
	ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
	ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

	RenderGraphResource *DepthBufferTexture = renderGraph->CreateTexture(ImageCreateInfo, "DepthBufferTexture");

	GBufferOpaquePass = renderGraph->CreateRenderPass<ScenePass>("G-Buffer Opaque Pass");

	GBufferOpaquePass->AddOutput(GBufferTexture0);
	GBufferOpaquePass->AddOutput(GBufferTexture1);
	GBufferOpaquePass->AddOutput(DepthBufferTexture);
}

void GBufferOpaqueStage::Execute()
{
	
}