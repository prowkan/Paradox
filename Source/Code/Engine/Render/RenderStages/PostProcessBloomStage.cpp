// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "PostProcessBloomStage.h"

#include "../RenderGraph.h"

void PostProcessBloomStage::Init(RenderGraph* renderGraph)
{
	uint32_t ResolutionWidth = 1280;
	uint32_t ResolutionHeight = 720;

	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	//ImageCreateInfo.extent.height = ResolutionHeight;
	//ImageCreateInfo.extent.width = ResolutionWidth;
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

	RenderGraphResource *BloomTextures[3][7];

	for (int i = 0; i < 7; i++)
	{
		ImageCreateInfo.extent.height = ResolutionHeight >> i;
		ImageCreateInfo.extent.width = ResolutionWidth >> i;

		/*SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[0][i]));
		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[1][i]));
		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[2][i]));*/

		BloomTextures[0][i] = renderGraph->CreateTexture(ImageCreateInfo, String("BloomTextures") + String(0) + String(i));
		BloomTextures[1][i] = renderGraph->CreateTexture(ImageCreateInfo, String("BloomTextures") + String(1) + String(i));
		BloomTextures[2][i] = renderGraph->CreateTexture(ImageCreateInfo, String("BloomTextures") + String(2) + String(i));
	}

	BloomPasses = new FullScreenPass*[3 * 7 + 6];

	for (int i = 0; i < 3 * 7 + 6; i++)
	{
		BloomPasses[i] = renderGraph->CreateRenderPass<FullScreenPass>(String("Post-Process Bloom Pass ") + String(i));
	}

	BloomPasses[0]->AddInput(renderGraph->GetResource("ResolvedHDRSceneColorTexture"));
	BloomPasses[0]->AddInput(renderGraph->GetResource("SceneLuminanceTexture0"));
	BloomPasses[0]->AddOutput(BloomTextures[0][0]);

	BloomPasses[1]->AddInput(BloomTextures[0][0]);
	BloomPasses[1]->AddOutput(BloomTextures[1][0]);

	BloomPasses[2]->AddInput(BloomTextures[1][0]);
	BloomPasses[2]->AddOutput(BloomTextures[2][0]);

	int Index = 3;

	for (int i = 0; i < 6; i++)
	{
		BloomPasses[Index]->AddInput(BloomTextures[0][i]);
		BloomPasses[Index]->AddOutput(BloomTextures[0][i + 1]);

		Index++;

		BloomPasses[Index]->AddInput(BloomTextures[0][i + 1]);
		BloomPasses[Index]->AddOutput(BloomTextures[1][i + 1]);

		Index++;

		BloomPasses[Index]->AddInput(BloomTextures[1][i + 1]);
		BloomPasses[Index]->AddOutput(BloomTextures[2][i + 1]);

		Index++;
	}

	for (int i = 5; i >= 0; i--)
	{
		BloomPasses[Index]->AddInput(BloomTextures[2][i + 1]);
		BloomPasses[Index]->AddOutput(BloomTextures[2][i]);

		Index++;
	}

	for (int i = 0; i < 3 * 7 + 6; i++)
	{
		BloomPasses[i]->SetExecutionCallBack([=] () -> void
		{
			cout << "BloomPass[" << i << "] callback was called." << endl;
		});
	}
}

void PostProcessBloomStage::Execute()
{
	for (int i = 0; i < 3 * 7 + 6; i++)
	{
		BloomPasses[i]->ExecutePass();
	}
}