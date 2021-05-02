// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessLuminanceStage.h"

#include "../RenderGraph.h"

void PostProcessLuminanceStage::Init(RenderGraph* renderGraph)
{
	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	//ImageCreateInfo.extent.height = ResolutionHeight;
	//ImageCreateInfo.extent.width = ResolutionWidth;
	ImageCreateInfo.flags = 0;
	ImageCreateInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
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
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

	int Widths[4] = { 1280, 80, 5, 1 };
	int Heights[4] = { 720, 45, 3, 1 };

	RenderGraphResource *SceneLuminanceTextures[4];

	for (int i = 0; i < 4; i++)
	{
		ImageCreateInfo.extent.height = Heights[i];
		ImageCreateInfo.extent.width = Widths[i];

		//SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &SceneLuminanceTextures[i]));
		SceneLuminanceTextures[i] = renderGraph->CreateTexture(ImageCreateInfo, String("SceneLuminanceTexture") + String(i));
	}

	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.extent.height = 1;
	ImageCreateInfo.extent.width = 1;
	ImageCreateInfo.flags = 0;
	ImageCreateInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
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
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

	//SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &AverageLuminanceTexture));
	RenderGraphResource *AverageLuminanceTexture = renderGraph->CreateTexture(ImageCreateInfo, "AverageLuminanceTexture");

	SceneLuminancePasses = new ComputePass*[5];

	SceneLuminancePasses[0] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 0");
	SceneLuminancePasses[1] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 1");
	SceneLuminancePasses[2] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 2");
	SceneLuminancePasses[3] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 3");
	SceneLuminancePasses[4] = renderGraph->CreateRenderPass<ComputePass>("Post-Process Scene Luminance Pass 4");

	SceneLuminancePasses[0]->AddInput(renderGraph->GetResource("ResolvedHDRSceneColorTexture"));
	SceneLuminancePasses[0]->AddOutput(SceneLuminanceTextures[0]);

	for (int i = 1; i < 4; i++)
	{
		SceneLuminancePasses[i]->AddInput(SceneLuminanceTextures[i - 1]);
		SceneLuminancePasses[i]->AddOutput(SceneLuminanceTextures[i]);
	}

	SceneLuminancePasses[4]->AddInput(SceneLuminanceTextures[3]);
	SceneLuminancePasses[4]->AddOutput(AverageLuminanceTexture);
}

void PostProcessLuminanceStage::Execute()
{
	
}