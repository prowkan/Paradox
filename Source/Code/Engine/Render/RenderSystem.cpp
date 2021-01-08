#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/GameObjects/Render/Meshes/StaticMeshObject.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

#ifdef _DEBUG
VkBool32 VKAPI_PTR vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,	VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) OutputDebugString(L"[ERROR]");
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) OutputDebugString(L"[INFO]");
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) OutputDebugString(L"[VERBOSE]");
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) OutputDebugString(L"[WARNING]");

	OutputDebugString(L" ");

	if (messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) OutputDebugString(L"[GENERAL]");
	if (messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) OutputDebugString(L"[PERFORMANCE]");
	if (messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) OutputDebugString(L"[VALIDATION]");

	OutputDebugString(L" ");

	wchar_t *Message = new wchar_t[strlen(pCallbackData->pMessage) + 1];

	for (size_t i = 0; pCallbackData->pMessage[i] != 0; i++)
	{
		Message[i] = pCallbackData->pMessage[i];
	}

	Message[strlen(pCallbackData->pMessage)] = 0;

	OutputDebugString(Message);

	delete[] Message;

	OutputDebugString(L"\r\n");

	return VK_FALSE;
}
#endif

void RenderSystem::InitSystem()
{
	VkResult Result;

	uint32_t APIVersion;

	Result = vkEnumerateInstanceVersion(&APIVersion);

	uint32_t InstanceLayerPropertiesCount;
	Result = vkEnumerateInstanceLayerProperties(&InstanceLayerPropertiesCount, nullptr);
	VkLayerProperties *InstanceLayerProperties = new VkLayerProperties[InstanceLayerPropertiesCount];
	Result = vkEnumerateInstanceLayerProperties(&InstanceLayerPropertiesCount, InstanceLayerProperties);

	uint32_t InstanceExtensionPropertiesCount;
	Result = vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionPropertiesCount, nullptr);
	VkExtensionProperties *InstanceExtensionProperties = new VkExtensionProperties[InstanceExtensionPropertiesCount];
	Result = vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionPropertiesCount, InstanceExtensionProperties);
	delete[] InstanceExtensionProperties;

	for (uint32_t i = 0; i < InstanceLayerPropertiesCount; i++)
	{
		Result = vkEnumerateInstanceExtensionProperties(InstanceLayerProperties[i].layerName, &InstanceExtensionPropertiesCount, nullptr);
		VkExtensionProperties *InstanceExtensionProperties = new VkExtensionProperties[InstanceExtensionPropertiesCount];
		Result = vkEnumerateInstanceExtensionProperties(InstanceLayerProperties[i].layerName, &InstanceExtensionPropertiesCount, InstanceExtensionProperties);
		delete[] InstanceExtensionProperties;
	}

	delete[] InstanceLayerProperties;

	VkApplicationInfo ApplicationInfo;
	ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pApplicationName = "Tolerance Paradox";
	ApplicationInfo.pEngineName = "Paradox Engine";
	ApplicationInfo.pNext = nullptr;
	ApplicationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;

#ifdef _DEBUG
	uint32_t EnabledInstanceExtensionsCount = 3;
	uint32_t EnabledInstanceLayersCount = 1;

	const char* EnabledInstanceExtensionsNames[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const char* EnabledInstanceLayersNames[] = { "VK_LAYER_KHRONOS_validation" };
#else
	uint32_t EnabledInstanceExtensionsCount = 2;
	uint32_t EnabledInstanceLayersCount = 0;

	const char* EnabledInstanceExtensionsNames[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	const char** EnabledInstanceLayersNames = nullptr;
#endif

	VkInstanceCreateInfo InstanceCreateInfo;
	InstanceCreateInfo.enabledExtensionCount = EnabledInstanceExtensionsCount;
	InstanceCreateInfo.enabledLayerCount = EnabledInstanceLayersCount;
	InstanceCreateInfo.flags = 0;
	InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;
	InstanceCreateInfo.pNext = nullptr;
	InstanceCreateInfo.ppEnabledExtensionNames = EnabledInstanceExtensionsNames;
	InstanceCreateInfo.ppEnabledLayerNames = EnabledInstanceLayersNames;
	InstanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	
	Result = vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance);

#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerCreateInfo;
	DebugUtilsMessengerCreateInfo.flags = 0;
	DebugUtilsMessengerCreateInfo.messageSeverity = VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	DebugUtilsMessengerCreateInfo.messageType = VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	DebugUtilsMessengerCreateInfo.pfnUserCallback = &vkDebugUtilsMessengerCallbackEXT;
	DebugUtilsMessengerCreateInfo.pNext = nullptr;
	DebugUtilsMessengerCreateInfo.pUserData = nullptr;
	DebugUtilsMessengerCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");

	Result = vkCreateDebugUtilsMessengerEXT(Instance, &DebugUtilsMessengerCreateInfo, nullptr, &DebugUtilsMessenger);
#endif

	uint32_t PhysicalDevicesCount;
	Result = vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, nullptr);
	VkPhysicalDevice *PhysicalDevices = new VkPhysicalDevice[PhysicalDevicesCount];
	Result = vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, PhysicalDevices);

	for (uint32_t i = 0; i < PhysicalDevicesCount; i++)
	{
		VkPhysicalDeviceProperties PhysicalDeviceProperties;

		vkGetPhysicalDeviceProperties(PhysicalDevices[i], &PhysicalDeviceProperties);

		if (PhysicalDeviceProperties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			PhysicalDevice = PhysicalDevices[i];
			break;
		}
	}

	delete[] PhysicalDevices;

	uint32_t DeviceLayerPropertiesCount;
	Result = vkEnumerateDeviceLayerProperties(PhysicalDevice, &DeviceLayerPropertiesCount, nullptr);
	VkLayerProperties *DeviceLayerProperties = new VkLayerProperties[DeviceLayerPropertiesCount];
	Result = vkEnumerateDeviceLayerProperties(PhysicalDevice, &DeviceLayerPropertiesCount, DeviceLayerProperties);

	uint32_t DeviceExtensionPropertiesCount;
	Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionPropertiesCount, nullptr);
	VkExtensionProperties *DeviceExtensionProperties = new VkExtensionProperties[DeviceExtensionPropertiesCount];
	Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionPropertiesCount, DeviceExtensionProperties);
	delete[] DeviceExtensionProperties;

	for (uint32_t i = 0; i < DeviceLayerPropertiesCount; i++)
	{
		Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, DeviceLayerProperties[i].layerName, &DeviceExtensionPropertiesCount, nullptr);
		VkExtensionProperties *DeviceExtensionProperties = new VkExtensionProperties[DeviceExtensionPropertiesCount];
		Result = vkEnumerateDeviceExtensionProperties(PhysicalDevice, DeviceLayerProperties[i].layerName, &DeviceExtensionPropertiesCount, DeviceExtensionProperties);
		delete[] DeviceExtensionProperties;
	}

	delete[] DeviceLayerProperties;

	VkPhysicalDeviceFeatures AvailablePhysicalDeviceFeatures, EnabledPhysicalDeviceFeatures;

	vkGetPhysicalDeviceFeatures(PhysicalDevice, &AvailablePhysicalDeviceFeatures);

	ZeroMemory(&EnabledPhysicalDeviceFeatures, sizeof(VkPhysicalDeviceFeatures));
	EnabledPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

	uint32_t QueueFamilyPropertiesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertiesCount, nullptr);
	VkQueueFamilyProperties *QueueFamilyProperties = new VkQueueFamilyProperties[QueueFamilyPropertiesCount];
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyPropertiesCount, QueueFamilyProperties);

	float QueuePriority = 1.0f;

	uint32_t QueueFamilyIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < QueueFamilyPropertiesCount; i++)
		{
			if (QueueFamilyProperties[i].queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
			{
				return i;
			}
		}

		return -1;

	} ();

	VkDeviceQueueCreateInfo DeviceQueueCreateInfo;
	DeviceQueueCreateInfo.flags = 0;
	DeviceQueueCreateInfo.pNext = nullptr;
	DeviceQueueCreateInfo.pQueuePriorities = &QueuePriority;
	DeviceQueueCreateInfo.queueCount = 1;
	DeviceQueueCreateInfo.queueFamilyIndex = QueueFamilyIndex;
	DeviceQueueCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

	uint32_t EnabledDeviceExtensionsCount = 1;

	const char* EnabledDeviceExtensionsNames[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo DeviceCreateInfo;
	DeviceCreateInfo.enabledExtensionCount = EnabledDeviceExtensionsCount;
	DeviceCreateInfo.enabledLayerCount = 0;
	DeviceCreateInfo.flags = 0;
	DeviceCreateInfo.pEnabledFeatures = &EnabledPhysicalDeviceFeatures;
	DeviceCreateInfo.pNext = nullptr;
	DeviceCreateInfo.ppEnabledExtensionNames = EnabledDeviceExtensionsNames;
	DeviceCreateInfo.ppEnabledLayerNames = nullptr;
	DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	
	Result = vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device);

	VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;

	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalDeviceMemoryProperties);

	VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo;
	Win32SurfaceCreateInfo.flags = 0;
	Win32SurfaceCreateInfo.hinstance = GetModuleHandle(NULL);
	Win32SurfaceCreateInfo.hwnd = Application::GetMainWindowHandle();
	Win32SurfaceCreateInfo.pNext = nullptr;
	Win32SurfaceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

	Result = vkCreateWin32SurfaceKHR(Instance, &Win32SurfaceCreateInfo, nullptr, &Surface);

	VkSurfaceCapabilitiesKHR SurfaceCapabilities;

	Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities);

	uint32_t SurfaceFormatsCount;
	Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatsCount, nullptr);
	VkSurfaceFormatKHR *SurfaceFormats = new VkSurfaceFormatKHR[SurfaceFormatsCount];
	Result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatsCount, SurfaceFormats);

	uint32_t PresentModesCount;
	Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModesCount, nullptr);
	VkPresentModeKHR *PresentModes = new VkPresentModeKHR[PresentModesCount];
	Result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModesCount, PresentModes);

	VkBool32 SurfaceSupport;
	Result = vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, QueueFamilyIndex, Surface, &SurfaceSupport);

	ResolutionWidth = SurfaceCapabilities.currentExtent.width;
	ResolutionHeight = SurfaceCapabilities.currentExtent.height;

	VkSwapchainCreateInfoKHR SwapchainCreateInfo;
	SwapchainCreateInfo.clipped = VK_FALSE;
	SwapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapchainCreateInfo.flags = 0;
	SwapchainCreateInfo.imageArrayLayers = 1;
	SwapchainCreateInfo.imageColorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	SwapchainCreateInfo.imageExtent.height = ResolutionHeight;
	SwapchainCreateInfo.imageExtent.width = ResolutionWidth;
	SwapchainCreateInfo.imageFormat = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
	SwapchainCreateInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	SwapchainCreateInfo.imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	SwapchainCreateInfo.minImageCount = 2;
	SwapchainCreateInfo.oldSwapchain = VK_FALSE;
	SwapchainCreateInfo.pNext = nullptr;
	SwapchainCreateInfo.pQueueFamilyIndices = nullptr;
	SwapchainCreateInfo.presentMode = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR;
	SwapchainCreateInfo.preTransform = VkSurfaceTransformFlagBitsKHR::VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	SwapchainCreateInfo.queueFamilyIndexCount = 0;
	SwapchainCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapchainCreateInfo.surface = Surface;

	Result = vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, nullptr, &SwapChain);

	vkGetDeviceQueue(Device, QueueFamilyIndex, 0, &CommandQueue);

	VkCommandPoolCreateInfo CommandPoolCreateInfo;
	CommandPoolCreateInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CommandPoolCreateInfo.pNext = nullptr;
	CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
	CommandPoolCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	Result = vkCreateCommandPool(Device, &CommandPoolCreateInfo, nullptr, &CommandPools[0]);
	Result = vkCreateCommandPool(Device, &CommandPoolCreateInfo, nullptr, &CommandPools[1]);

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo;
	CommandBufferAllocateInfo.commandBufferCount = 1;
	CommandBufferAllocateInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CommandBufferAllocateInfo.pNext = nullptr;
	CommandBufferAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	CommandBufferAllocateInfo.commandPool = CommandPools[0];
	Result = vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, &CommandBuffers[0]);
	CommandBufferAllocateInfo.commandPool = CommandPools[1];
	Result = vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, &CommandBuffers[1]);

	CurrentFrameIndex = 0;

	VkSemaphoreCreateInfo SemaphoreCreateInfo;
	SemaphoreCreateInfo.flags = 0;
	SemaphoreCreateInfo.pNext = nullptr;
	SemaphoreCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	Result = vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ImageAvailabilitySemaphore);
	Result = vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ImagePresentationSemaphore);

	VkFenceCreateInfo FenceCreateInfo;
	FenceCreateInfo.flags = 0;
	FenceCreateInfo.pNext = nullptr;
	FenceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
	Result = vkCreateFence(Device, &FenceCreateInfo, nullptr, &Fences[0]);
	
	FenceCreateInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

	Result = vkCreateFence(Device, &FenceCreateInfo, nullptr, &Fences[1]);

	VkDescriptorPoolSize DescriptorPoolSizes[3];
	DescriptorPoolSizes[0].descriptorCount = 20000;
	DescriptorPoolSizes[0].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescriptorPoolSizes[1].descriptorCount = 20000;
	DescriptorPoolSizes[1].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorPoolSizes[2].descriptorCount = 1;
	DescriptorPoolSizes[2].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;

	VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
	DescriptorPoolCreateInfo.flags = 0;
	DescriptorPoolCreateInfo.maxSets = 2 * 20000 + 1;
	DescriptorPoolCreateInfo.pNext = nullptr;
	DescriptorPoolCreateInfo.poolSizeCount = 3;
	DescriptorPoolCreateInfo.pPoolSizes = DescriptorPoolSizes;
	DescriptorPoolCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

	Result = vkCreateDescriptorPool(Device, &DescriptorPoolCreateInfo, nullptr, &DescriptorPools[0]);
	Result = vkCreateDescriptorPool(Device, &DescriptorPoolCreateInfo, nullptr, &DescriptorPools[1]);

	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;
	DescriptorSetLayoutBinding.binding = 0;
	DescriptorSetLayoutBinding.descriptorCount = 1;
	DescriptorSetLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescriptorSetLayoutBinding.pImmutableSamplers = nullptr;
	DescriptorSetLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
	DescriptorSetLayoutCreateInfo.bindingCount = 1;
	DescriptorSetLayoutCreateInfo.flags = 0;
	DescriptorSetLayoutCreateInfo.pBindings = &DescriptorSetLayoutBinding;
	DescriptorSetLayoutCreateInfo.pNext = nullptr;
	DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	
	Result = vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &ConstantBuffersSetLayout);

	DescriptorSetLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorSetLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

	Result = vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &TexturesSetLayout);

	DescriptorSetLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
	DescriptorSetLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

	Result = vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &SamplersSetLayout);

	VkDescriptorSetLayout DescriptorSetLayouts[3] = { ConstantBuffersSetLayout, TexturesSetLayout, SamplersSetLayout };

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
	PipelineLayoutCreateInfo.flags = 0;
	PipelineLayoutCreateInfo.pNext = nullptr;
	PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	PipelineLayoutCreateInfo.pSetLayouts = DescriptorSetLayouts;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	PipelineLayoutCreateInfo.setLayoutCount = 3;
	PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	Result = vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);

	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
	DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
	DescriptorSetAllocateInfo.descriptorSetCount = 1;
	DescriptorSetAllocateInfo.pNext = nullptr;
	DescriptorSetAllocateInfo.pSetLayouts = &SamplersSetLayout;
	DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SamplersSets[0]);

	DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];

	Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SamplersSets[1]);

	for (uint32_t i = 0; i < 20000; i++)
	{
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		DescriptorSetAllocateInfo.pSetLayouts = &ConstantBuffersSetLayout;
		
		Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[0][i]);

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];

		Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[1][i]);

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		DescriptorSetAllocateInfo.pSetLayouts = &TexturesSetLayout;

		Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &TexturesSets[0][i]);

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];

		Result = vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &TexturesSets[1][i]);
	}

	uint32_t SwapChainImagesCount;
	Result = vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImagesCount, nullptr);
	Result = vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImagesCount, BackBufferTextures);

	VkImageViewCreateInfo ImageViewCreateInfo;
	ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.flags = 0;
	ImageViewCreateInfo.format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
	ImageViewCreateInfo.pNext = nullptr;
	ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ImageViewCreateInfo.subresourceRange.layerCount = 1;
	ImageViewCreateInfo.subresourceRange.levelCount = 1;
	ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

	ImageViewCreateInfo.image = BackBufferTextures[0];
	Result = vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &BackBufferRTVs[0]);
	ImageViewCreateInfo.image = BackBufferTextures[1];
	Result = vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &BackBufferRTVs[1]);

	VkImageCreateInfo ImageCreateInfo;
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
	ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	Result = vkCreateImage(Device, &ImageCreateInfo, nullptr, &DepthBufferTexture);

	VkMemoryRequirements MemoryRequirements;

	vkGetImageMemoryRequirements(Device, DepthBufferTexture, &MemoryRequirements);

	VkMemoryAllocateInfo MemoryAllocateInfo;
	MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
	MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if (((1 << i) & MemoryRequirements.memoryTypeBits) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				return i;
		}

		return -1;

	} ();
	MemoryAllocateInfo.pNext = nullptr;
	MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &DepthBufferTextureMemoryHeap);

	Result = vkBindImageMemory(Device, DepthBufferTexture, DepthBufferTextureMemoryHeap, 0);

	ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.flags = 0;
	ImageViewCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
	ImageViewCreateInfo.image = DepthBufferTexture;
	ImageViewCreateInfo.pNext = nullptr;
	ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ImageViewCreateInfo.subresourceRange.layerCount = 1;
	ImageViewCreateInfo.subresourceRange.levelCount = 1;
	ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

	Result = vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &DepthBufferDSV);

	VkAttachmentDescription AttachmentDescriptions[2];
	AttachmentDescriptions[0].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	AttachmentDescriptions[0].flags = 0;
	AttachmentDescriptions[0].format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
	AttachmentDescriptions[0].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	AttachmentDescriptions[0].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
	AttachmentDescriptions[0].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	AttachmentDescriptions[0].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	AttachmentDescriptions[0].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
	AttachmentDescriptions[0].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
	AttachmentDescriptions[1].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	AttachmentDescriptions[1].flags = 0;
	AttachmentDescriptions[1].format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
	AttachmentDescriptions[1].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	AttachmentDescriptions[1].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
	AttachmentDescriptions[1].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	AttachmentDescriptions[1].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
	AttachmentDescriptions[1].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
	AttachmentDescriptions[1].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

	VkAttachmentReference AttachmentReferences[2];
	AttachmentReferences[0].attachment = 0;
	AttachmentReferences[0].layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	AttachmentReferences[1].attachment = 1;
	AttachmentReferences[1].layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription SubpassDescription;
	SubpassDescription.colorAttachmentCount = 1;
	SubpassDescription.flags = 0;
	SubpassDescription.inputAttachmentCount = 0;
	SubpassDescription.pColorAttachments = &AttachmentReferences[0];
	SubpassDescription.pDepthStencilAttachment = &AttachmentReferences[1];
	SubpassDescription.pInputAttachments = nullptr;
	SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDescription.pPreserveAttachments = nullptr;
	SubpassDescription.preserveAttachmentCount = 0;
	SubpassDescription.pResolveAttachments = nullptr;

	VkRenderPassCreateInfo RenderPassCreateInfo;
	RenderPassCreateInfo.attachmentCount = 2;
	RenderPassCreateInfo.dependencyCount = 0;
	RenderPassCreateInfo.flags = 0;
	RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
	RenderPassCreateInfo.pDependencies = nullptr;
	RenderPassCreateInfo.pNext = nullptr;
	RenderPassCreateInfo.pSubpasses = &SubpassDescription;
	RenderPassCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	RenderPassCreateInfo.subpassCount = 1;

	Result = vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &RenderPass);

	VkImageView FrameBufferAttachments[2] = { BackBufferRTVs[0], DepthBufferDSV };

	VkFramebufferCreateInfo FramebufferCreateInfo;
	FramebufferCreateInfo.attachmentCount = 2;
	FramebufferCreateInfo.flags = 0;
	FramebufferCreateInfo.height = ResolutionHeight;
	FramebufferCreateInfo.layers = 1;
	FramebufferCreateInfo.pAttachments = FrameBufferAttachments;
	FramebufferCreateInfo.pNext = nullptr;
	FramebufferCreateInfo.renderPass = RenderPass;
	FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	FramebufferCreateInfo.width = ResolutionWidth;

	Result = vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &FrameBuffers[0]);

	FrameBufferAttachments[0] = BackBufferRTVs[1];

	Result = vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &FrameBuffers[1]);

	VkBufferCreateInfo BufferCreateInfo;
	BufferCreateInfo.flags = 0;
	BufferCreateInfo.pNext = nullptr;
	BufferCreateInfo.pQueueFamilyIndices = nullptr;
	BufferCreateInfo.queueFamilyIndexCount = 0;
	BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	BufferCreateInfo.size = 64 * 20000;
	BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	Result = vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUConstantBuffer);

	vkGetBufferMemoryRequirements(Device, GPUConstantBuffer, &MemoryRequirements);

	MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
	MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if (((1 << i) & MemoryRequirements.memoryTypeBits) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				return i;
		}

		return -1;

	} ();
	MemoryAllocateInfo.pNext = nullptr;
	MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUConstantBufferMemoryHeap);

	Result = vkBindBufferMemory(Device, GPUConstantBuffer, GPUConstantBufferMemoryHeap, 0);

	BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	Result = vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers[0]);

	vkGetBufferMemoryRequirements(Device, GPUConstantBuffer, &MemoryRequirements);

	MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
	MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if (((1 << i) & MemoryRequirements.memoryTypeBits) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				return i;
		}

		return -1;

	} ();
	MemoryAllocateInfo.pNext = nullptr;
	MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUConstantBufferMemoryHeaps[0]);

	Result = vkBindBufferMemory(Device, CPUConstantBuffers[0], CPUConstantBufferMemoryHeaps[0], 0);

	Result = vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers[1]);

	vkGetBufferMemoryRequirements(Device, GPUConstantBuffer, &MemoryRequirements);

	MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
	MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if (((1 << i) & MemoryRequirements.memoryTypeBits) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				return i;
		}

		return -1;

	} ();
	MemoryAllocateInfo.pNext = nullptr;
	MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUConstantBufferMemoryHeaps[1]);

	Result = vkBindBufferMemory(Device, CPUConstantBuffers[1], CPUConstantBufferMemoryHeaps[1], 0);

	VkSamplerCreateInfo SamplerCreateInfo;
	SamplerCreateInfo.addressModeU = SamplerCreateInfo.addressModeV = SamplerCreateInfo.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	SamplerCreateInfo.anisotropyEnable = VK_TRUE;
	SamplerCreateInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	SamplerCreateInfo.compareEnable = VK_FALSE;
	SamplerCreateInfo.compareOp = (VkCompareOp)0;
	SamplerCreateInfo.flags = 0;
	SamplerCreateInfo.magFilter = VkFilter::VK_FILTER_LINEAR;
	SamplerCreateInfo.maxAnisotropy = 16.0f;
	SamplerCreateInfo.maxLod = FLT_MAX;
	SamplerCreateInfo.minFilter = VkFilter::VK_FILTER_LINEAR;
	SamplerCreateInfo.minLod = 0.0f;
	SamplerCreateInfo.mipLodBias = 0.0f;
	SamplerCreateInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
	SamplerCreateInfo.pNext = nullptr;
	SamplerCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	Result = vkCreateSampler(Device, &SamplerCreateInfo, nullptr, &Sampler);

	MemoryAllocateInfo.allocationSize = BUFFER_MEMORY_HEAP_SIZE;
	MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				return i;
		}

		return -1;

	} ();
	MemoryAllocateInfo.pNext = nullptr;
	MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]);

	MemoryAllocateInfo.allocationSize = TEXTURE_MEMORY_HEAP_SIZE;
	MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				return i;
		}

		return -1;

	} ();
	MemoryAllocateInfo.pNext = nullptr;
	MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &TextureMemoryHeaps[CurrentTextureMemoryHeapIndex]);

	BufferCreateInfo.flags = 0;
	BufferCreateInfo.pNext = nullptr;
	BufferCreateInfo.pQueueFamilyIndices = nullptr;
	BufferCreateInfo.queueFamilyIndexCount = 0;
	BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	BufferCreateInfo.size = UPLOAD_HEAP_SIZE;
	BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	Result = vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &UploadBuffer);

	vkGetBufferMemoryRequirements(Device, UploadBuffer, &MemoryRequirements);

	MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
	MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

		for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if (((1 << i) & MemoryRequirements.memoryTypeBits) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				return i;
		}

		return -1;

	} ();
	MemoryAllocateInfo.pNext = nullptr;
	MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

	Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &UploadHeap);

	Result = vkBindBufferMemory(Device, UploadBuffer, UploadHeap, 0);
}

void RenderSystem::ShutdownSystem()
{
	VkResult Result;

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;

	Result = vkWaitForFences(Device, 1, &Fences[CurrentFrameIndex], VK_FALSE, UINT64_MAX);

	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	{
		vkDestroyBuffer(Device, renderMesh->VertexBuffer, nullptr);
		vkDestroyBuffer(Device, renderMesh->IndexBuffer, nullptr);

		delete renderMesh;
	}

	RenderMeshDestructionQueue.clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	{
		vkDestroyPipeline(Device, renderMaterial->Pipeline, nullptr);

		delete renderMaterial;
	}

	RenderMaterialDestructionQueue.clear();

	for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	{
		vkDestroyImageView(Device, renderTexture->TextureView, nullptr);
		vkDestroyImage(Device, renderTexture->Texture, nullptr);

		delete renderTexture;
	}

	RenderTextureDestructionQueue.clear();

	for (int i = 0; i < MAX_MEMORY_HEAPS_COUNT; i++)
	{
		if (BufferMemoryHeaps[i] != VK_NULL_HANDLE) vkFreeMemory(Device, BufferMemoryHeaps[i], nullptr);
		if (TextureMemoryHeaps[i] != VK_NULL_HANDLE) vkFreeMemory(Device, TextureMemoryHeaps[i], nullptr);
	}

	vkDestroyBuffer(Device, UploadBuffer, nullptr);
	vkFreeMemory(Device, UploadHeap, nullptr);

	vkDestroyImageView(Device, BackBufferRTVs[0], nullptr);
	vkDestroyImageView(Device, BackBufferRTVs[1], nullptr);
	
	vkDestroyImageView(Device, DepthBufferDSV, nullptr);
	vkDestroyImage(Device, DepthBufferTexture, nullptr);
	vkFreeMemory(Device, DepthBufferTextureMemoryHeap, nullptr);

	vkDestroyFence(Device, Fences[0], nullptr);
	vkDestroyFence(Device, Fences[1], nullptr);
	vkDestroySemaphore(Device, ImageAvailabilitySemaphore, nullptr);
	vkDestroySemaphore(Device, ImagePresentationSemaphore, nullptr);

	vkDestroySampler(Device, Sampler, nullptr);

	vkDestroyBuffer(Device, GPUConstantBuffer, nullptr);
	vkFreeMemory(Device, GPUConstantBufferMemoryHeap, nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers[0], nullptr);
	vkFreeMemory(Device, CPUConstantBufferMemoryHeaps[0], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers[1], nullptr);
	vkFreeMemory(Device, CPUConstantBufferMemoryHeaps[1], nullptr);

	vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);

	vkDestroyDescriptorSetLayout(Device, ConstantBuffersSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, TexturesSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, SamplersSetLayout, nullptr);

	vkDestroyDescriptorPool(Device, DescriptorPools[0], nullptr);
	vkDestroyDescriptorPool(Device, DescriptorPools[1], nullptr);

	vkFreeCommandBuffers(Device, CommandPools[0], 1, &CommandBuffers[0]);
	vkFreeCommandBuffers(Device, CommandPools[1], 1, &CommandBuffers[1]);

	vkDestroyCommandPool(Device, CommandPools[0], nullptr);
	vkDestroyCommandPool(Device, CommandPools[1], nullptr);

	vkDestroySwapchainKHR(Device, SwapChain, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);

	vkDestroyFramebuffer(Device, FrameBuffers[0], nullptr);
	vkDestroyFramebuffer(Device, FrameBuffers[1], nullptr);
	vkDestroyRenderPass(Device, RenderPass, nullptr);

#ifdef _DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");

	vkDestroyDebugUtilsMessengerEXT(Instance, DebugUtilsMessenger, nullptr);
#endif

	vkDestroyDevice(Device, nullptr);
	vkDestroyInstance(Instance, nullptr);
}

void RenderSystem::TickSystem(float DeltaTime)
{
	VkResult Result;

	Result = vkAcquireNextImageKHR(Device, SwapChain, UINT64_MAX, ImageAvailabilitySemaphore, VK_NULL_HANDLE, &CurrentBackBufferIndex);

	VkCommandBufferBeginInfo CommandBufferBeginInfo;
	CommandBufferBeginInfo.flags = 0;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;
	CommandBufferBeginInfo.pNext = nullptr;
	CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	Result = vkBeginCommandBuffer(CommandBuffers[CurrentFrameIndex], &CommandBufferBeginInfo);

	XMMATRIX ViewProjMatrix = Engine::GetEngine().GetGameFramework().GetCamera().GetViewProjMatrix();

	void *ConstantBufferData;
	SIZE_T ConstantBufferOffset = 0;

	vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	OPTICK_EVENT("Draw Calls")

	Result = vkMapMemory(Device, CPUConstantBufferMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &ConstantBufferData);
	
	for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
		XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;

		memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset, &WVPMatrix, sizeof(XMMATRIX));

		ConstantBufferOffset += 64;
	}

	vkUnmapMemory(Device, CPUConstantBufferMemoryHeaps[CurrentFrameIndex]);

	VkBufferMemoryBarrier BufferMemoryBarrier;
	BufferMemoryBarrier.buffer = CPUConstantBuffers[CurrentFrameIndex];
	BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
	BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.offset = 0;
	BufferMemoryBarrier.pNext = nullptr;
	BufferMemoryBarrier.size = VK_WHOLE_SIZE;
	BufferMemoryBarrier.srcAccessMask = 0;
	BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	
	vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

	if (ConstantBufferOffset > 0)
	{
		VkBufferCopy BufferCopy;
		BufferCopy.dstOffset = 0;
		BufferCopy.size = ConstantBufferOffset;
		BufferCopy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPUConstantBuffers[CurrentFrameIndex], GPUConstantBuffer, 1, &BufferCopy);
	}

	BufferMemoryBarrier.buffer = GPUConstantBuffer;
	BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT;
	BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.offset = 0;
	BufferMemoryBarrier.pNext = nullptr;
	BufferMemoryBarrier.size = VK_WHOLE_SIZE;
	BufferMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
	BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

	VkImageMemoryBarrier ImageMemoryBarriers[2];
	ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarriers[0].image = BackBufferTextures[CurrentBackBufferIndex];
	ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	ImageMemoryBarriers[0].pNext = nullptr;
	ImageMemoryBarriers[0].srcAccessMask = 0;
	ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
	ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
	ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
	ImageMemoryBarriers[0].subresourceRange.levelCount = 1;
	ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarriers[1].image = DepthBufferTexture;
	ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	ImageMemoryBarriers[1].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	ImageMemoryBarriers[1].pNext = nullptr;
	ImageMemoryBarriers[1].srcAccessMask = 0;
	ImageMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImageMemoryBarriers[1].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
	ImageMemoryBarriers[1].subresourceRange.baseArrayLayer = 0;
	ImageMemoryBarriers[1].subresourceRange.baseMipLevel = 0;
	ImageMemoryBarriers[1].subresourceRange.layerCount = 1;
	ImageMemoryBarriers[1].subresourceRange.levelCount = 1;

	vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 2, ImageMemoryBarriers);

	VkClearValue ClearValues[2];
	ClearValues[0].color.float32[0] = 0.0f;
	ClearValues[0].color.float32[1] = 0.5f;
	ClearValues[0].color.float32[2] = 1.0f;
	ClearValues[0].color.float32[3] = 1.0f;
	ClearValues[1].depthStencil.depth = 1.0f;
	ClearValues[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo RenderPassBeginInfo;
	RenderPassBeginInfo.clearValueCount = 2;
	RenderPassBeginInfo.framebuffer = FrameBuffers[CurrentBackBufferIndex];
	RenderPassBeginInfo.pClearValues = ClearValues;
	RenderPassBeginInfo.pNext = nullptr;
	RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
	RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
	RenderPassBeginInfo.renderArea.offset.x = 0;
	RenderPassBeginInfo.renderArea.offset.y = 0;
	RenderPassBeginInfo.renderPass = RenderPass;
	RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

	VkViewport Viewport;
	Viewport.height = float(ResolutionHeight);
	Viewport.maxDepth = 1.0f;
	Viewport.minDepth = 0.0f;
	Viewport.x = 0.0f;
	Viewport.y = 0.0f;
	Viewport.width = float(ResolutionWidth);

	vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

	VkRect2D ScissorRect;
	ScissorRect.extent.height = ResolutionHeight;
	ScissorRect.offset.x = 0;
	ScissorRect.extent.width = ResolutionWidth;
	ScissorRect.offset.y = 0;

	vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

	VkDescriptorImageInfo DescriptorImageInfo;
	DescriptorImageInfo.imageLayout = (VkImageLayout)0;
	DescriptorImageInfo.imageView = VK_NULL_HANDLE;
	DescriptorImageInfo.sampler = Sampler;

	VkWriteDescriptorSet WriteDescriptorSet;
	WriteDescriptorSet.descriptorCount = 1;
	WriteDescriptorSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
	WriteDescriptorSet.dstArrayElement = 0;
	WriteDescriptorSet.dstBinding = 0;
	WriteDescriptorSet.dstSet = SamplersSets[CurrentFrameIndex];
	WriteDescriptorSet.pBufferInfo = nullptr;
	WriteDescriptorSet.pImageInfo = &DescriptorImageInfo;
	WriteDescriptorSet.pNext = nullptr;
	WriteDescriptorSet.pTexelBufferView = nullptr;
	WriteDescriptorSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

	vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);

	vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 2, 1, &SamplersSets[CurrentFrameIndex], 0, nullptr);

	VkDeviceSize Offset = 0;

	for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
	{
		StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

		RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
		RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
		RenderTexture *renderTexture = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();

		VkDescriptorBufferInfo DescriptorBufferInfo;
		DescriptorBufferInfo.buffer = GPUConstantBuffer;
		DescriptorBufferInfo.offset = 64 * k;
		DescriptorBufferInfo.range = 64;

		VkDescriptorImageInfo DescriptorImageInfo;
		DescriptorImageInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfo.imageView = renderTexture->TextureView;
		DescriptorImageInfo.sampler = VK_NULL_HANDLE;

		VkWriteDescriptorSet WriteDescriptorSets[2];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = ConstantBuffersSets[CurrentFrameIndex][k];
		WriteDescriptorSets[0].pBufferInfo = &DescriptorBufferInfo;
		WriteDescriptorSets[0].pImageInfo = nullptr;
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 0;
		WriteDescriptorSets[1].dstSet = TexturesSets[CurrentFrameIndex][k];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfo;
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindVertexBuffers(CommandBuffers[CurrentFrameIndex], 0, 1, &renderMesh->VertexBuffer, &Offset);
		vkCmdBindIndexBuffer(CommandBuffers[CurrentFrameIndex], renderMesh->IndexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, renderMaterial->Pipeline);

		VkDescriptorSet DescriptorSets[2] =
		{
			ConstantBuffersSets[CurrentFrameIndex][k],
			TexturesSets[CurrentFrameIndex][k]
		};
		
		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 2, DescriptorSets, 0, nullptr);

		vkCmdDrawIndexed(CommandBuffers[CurrentFrameIndex], 8 * 8 * 6 * 6, 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

	ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
	ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarriers[0].image = BackBufferTextures[CurrentBackBufferIndex];
	ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	ImageMemoryBarriers[0].pNext = nullptr;
	ImageMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
	ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
	ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
	ImageMemoryBarriers[0].subresourceRange.levelCount = 1;

	vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, ImageMemoryBarriers);

	Result = vkEndCommandBuffer(CommandBuffers[CurrentFrameIndex]);

	VkPipelineStageFlags WaitDstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	VkSubmitInfo SubmitInfo;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffers[CurrentFrameIndex];
	SubmitInfo.pNext = nullptr;
	SubmitInfo.pSignalSemaphores = &ImagePresentationSemaphore;
	SubmitInfo.pWaitDstStageMask = &WaitDstStageMask;
	SubmitInfo.pWaitSemaphores = &ImageAvailabilitySemaphore;
	SubmitInfo.signalSemaphoreCount = 1;
	SubmitInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.waitSemaphoreCount = 1;

	Result = vkQueueSubmit(CommandQueue, 1, &SubmitInfo, Fences[CurrentFrameIndex]);

	VkPresentInfoKHR PresentInfo;
	PresentInfo.pImageIndices = &CurrentBackBufferIndex;
	PresentInfo.pNext = nullptr;
	PresentInfo.pResults = nullptr;
	PresentInfo.pSwapchains = &SwapChain;
	PresentInfo.pWaitSemaphores = &ImagePresentationSemaphore;
	PresentInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	PresentInfo.swapchainCount = 1;
	PresentInfo.waitSemaphoreCount = 1;

	Result = vkQueuePresentKHR(CommandQueue, &PresentInfo);

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
	
	Result = vkWaitForFences(Device, 1, &Fences[CurrentFrameIndex], VK_FALSE, UINT64_MAX);

	Result = vkResetFences(Device, 1, &Fences[CurrentFrameIndex]);
}

RenderMesh* RenderSystem::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMesh *renderMesh = new RenderMesh();

	VkResult Result;

	VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;

	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalDeviceMemoryProperties);

	VkBufferCreateInfo BufferCreateInfo;
	BufferCreateInfo.flags = 0;
	BufferCreateInfo.pNext = nullptr;
	BufferCreateInfo.pQueueFamilyIndices = nullptr;
	BufferCreateInfo.queueFamilyIndexCount = 0;
	BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	BufferCreateInfo.size = sizeof(Vertex) * renderMeshCreateInfo.VertexCount;
	BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	Result = vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &renderMesh->VertexBuffer);

	VkMemoryRequirements MemoryRequirements;

	vkGetBufferMemoryRequirements(Device, renderMesh->VertexBuffer, &MemoryRequirements);

	size_t AlignedResourceOffset = BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] + (MemoryRequirements.alignment - BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] % MemoryRequirements.alignment);

	if (AlignedResourceOffset + MemoryRequirements.size > BUFFER_MEMORY_HEAP_SIZE)
	{
		++CurrentBufferMemoryHeapIndex;

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = BUFFER_MEMORY_HEAP_SIZE;
		MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

			for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
			{
				if (((1 << i) & MemoryRequirements.memoryTypeBits) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
					return i;
			}

			return -1;

		} ();
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]);

		AlignedResourceOffset = 0;
	}

	Result = vkBindBufferMemory(Device, renderMesh->VertexBuffer, BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset);

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + MemoryRequirements.size;

	BufferCreateInfo.flags = 0;
	BufferCreateInfo.pNext = nullptr;
	BufferCreateInfo.pQueueFamilyIndices = nullptr;
	BufferCreateInfo.queueFamilyIndexCount = 0;
	BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	BufferCreateInfo.size = sizeof(WORD) * renderMeshCreateInfo.IndexCount;
	BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	Result = vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &renderMesh->IndexBuffer);

	vkGetBufferMemoryRequirements(Device, renderMesh->IndexBuffer, &MemoryRequirements);

	AlignedResourceOffset = BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] + (MemoryRequirements.alignment - BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] % MemoryRequirements.alignment);

	if (AlignedResourceOffset + MemoryRequirements.size > BUFFER_MEMORY_HEAP_SIZE)
	{
		++CurrentBufferMemoryHeapIndex;

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = BUFFER_MEMORY_HEAP_SIZE;
		MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

			for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
			{
				if (((1 << i) & MemoryRequirements.memoryTypeBits) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
					return i;
			}

			return -1;

		} ();
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]);

		AlignedResourceOffset = 0;
	}

	Result = vkBindBufferMemory(Device, renderMesh->IndexBuffer, BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset);

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + MemoryRequirements.size;
	
	void *MappedData;

	Result = vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData);
	memcpy((BYTE*)MappedData, renderMeshCreateInfo.VertexData, sizeof(Vertex) * renderMeshCreateInfo.VertexCount);
	memcpy((BYTE*)MappedData + sizeof(Vertex) * renderMeshCreateInfo.VertexCount, renderMeshCreateInfo.IndexData, sizeof(WORD) * renderMeshCreateInfo.IndexCount);
	vkUnmapMemory(Device, UploadHeap);

	VkCommandBufferBeginInfo CommandBufferBeginInfo;
	CommandBufferBeginInfo.flags = 0;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;
	CommandBufferBeginInfo.pNext = nullptr;
	CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	Result = vkBeginCommandBuffer(CommandBuffers[0], &CommandBufferBeginInfo);

	VkBufferMemoryBarrier BufferMemoryBarriers[2];
	BufferMemoryBarriers[0].buffer = UploadBuffer;
	BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
	BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarriers[0].offset = 0;
	BufferMemoryBarriers[0].pNext = nullptr;
	BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
	BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
	BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

	vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, BufferMemoryBarriers, 0, nullptr);

	VkBufferCopy BufferCopy;
	BufferCopy.dstOffset = 0;
	BufferCopy.size = sizeof(Vertex) * renderMeshCreateInfo.VertexCount;
	BufferCopy.srcOffset = 0;

	vkCmdCopyBuffer(CommandBuffers[0], UploadBuffer, renderMesh->VertexBuffer, 1, &BufferCopy);

	BufferCopy.dstOffset = 0;
	BufferCopy.size = sizeof(WORD) * renderMeshCreateInfo.IndexCount;
	BufferCopy.srcOffset = sizeof(Vertex) * renderMeshCreateInfo.VertexCount;

	vkCmdCopyBuffer(CommandBuffers[0], UploadBuffer, renderMesh->IndexBuffer, 1, &BufferCopy);

	BufferMemoryBarriers[0].buffer = renderMesh->VertexBuffer;
	BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarriers[0].offset = 0;
	BufferMemoryBarriers[0].pNext = nullptr;
	BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
	BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
	BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	BufferMemoryBarriers[1].buffer = renderMesh->VertexBuffer;
	BufferMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT;
	BufferMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarriers[1].offset = 0;
	BufferMemoryBarriers[1].pNext = nullptr;
	BufferMemoryBarriers[1].size = VK_WHOLE_SIZE;
	BufferMemoryBarriers[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
	BufferMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

	vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 2, BufferMemoryBarriers, 0, nullptr);

	Result = vkEndCommandBuffer(CommandBuffers[0]);

	VkSubmitInfo SubmitInfo;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffers[0];
	SubmitInfo.pNext = nullptr;
	SubmitInfo.pSignalSemaphores = nullptr;
	SubmitInfo.pWaitDstStageMask = nullptr;
	SubmitInfo.pWaitSemaphores = nullptr;
	SubmitInfo.signalSemaphoreCount = 0;
	SubmitInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.waitSemaphoreCount = 0;

	Result = vkQueueSubmit(CommandQueue, 1, &SubmitInfo, VK_NULL_HANDLE);

	Result = vkQueueWaitIdle(CommandQueue);

	return renderMesh;
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTexture *renderTexture = new RenderTexture();

	VkResult Result;

	VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;

	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalDeviceMemoryProperties);

	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.extent.height = renderTextureCreateInfo.Height;
	ImageCreateInfo.extent.width = renderTextureCreateInfo.Width;
	ImageCreateInfo.flags = 0;
	ImageCreateInfo.format = renderTextureCreateInfo.SRGB ? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	ImageCreateInfo.imageType = VkImageType::VK_IMAGE_TYPE_2D;
	ImageCreateInfo.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.mipLevels = renderTextureCreateInfo.MIPLevels;
	ImageCreateInfo.pNext = nullptr;
	ImageCreateInfo.pQueueFamilyIndices = nullptr;
	ImageCreateInfo.queueFamilyIndexCount = 0;
	ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	ImageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	Result = vkCreateImage(Device, &ImageCreateInfo, nullptr, &renderTexture->Texture);

	VkMemoryRequirements MemoryRequirements;

	vkGetImageMemoryRequirements(Device, renderTexture->Texture, &MemoryRequirements);

	size_t AlignedResourceOffset = TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] + (MemoryRequirements.alignment - TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] % MemoryRequirements.alignment);

	if (AlignedResourceOffset + MemoryRequirements.size > TEXTURE_MEMORY_HEAP_SIZE)
	{
		++CurrentTextureMemoryHeapIndex;

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = TEXTURE_MEMORY_HEAP_SIZE;
		MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

			for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
			{
				if (((1 << i) & MemoryRequirements.memoryTypeBits) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
					return i;
			}

			return -1;

		} ();
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		Result = vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &TextureMemoryHeaps[CurrentTextureMemoryHeapIndex]);

		AlignedResourceOffset = 0;
	}

	Result = vkBindImageMemory(Device, renderTexture->Texture, TextureMemoryHeaps[CurrentTextureMemoryHeapIndex], AlignedResourceOffset);

	TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] = AlignedResourceOffset + MemoryRequirements.size;

	void *MappedData;

	Result = vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData);

	BYTE *TexelData = renderTextureCreateInfo.TexelData;

	for (UINT i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
	{
		for (UINT j = 0; j < renderTextureCreateInfo.Height >> i; j++)
		{
			memcpy((BYTE*)MappedData + j * 4 * (renderTextureCreateInfo.Width >> i), (BYTE*)TexelData + j * 4 * (renderTextureCreateInfo.Width >> i), 4 * (renderTextureCreateInfo.Width >> i));
		}

		TexelData += 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
		MappedData = (BYTE*)MappedData + 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
	}

	vkUnmapMemory(Device, UploadHeap);

	VkCommandBufferBeginInfo CommandBufferBeginInfo;
	CommandBufferBeginInfo.flags = 0;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;
	CommandBufferBeginInfo.pNext = nullptr;
	CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	Result = vkBeginCommandBuffer(CommandBuffers[0], &CommandBufferBeginInfo);

	VkBufferMemoryBarrier BufferMemoryBarrier;
	BufferMemoryBarrier.buffer = UploadBuffer;
	BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
	BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.offset = 0;
	BufferMemoryBarrier.pNext = nullptr;
	BufferMemoryBarrier.size = VK_WHOLE_SIZE;
	BufferMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
	BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

	VkImageMemoryBarrier ImageMemoryBarrier;
	ImageMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
	ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrier.image = renderTexture->Texture;
	ImageMemoryBarrier.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ImageMemoryBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	ImageMemoryBarrier.pNext = nullptr;
	ImageMemoryBarrier.srcAccessMask = 0;
	ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImageMemoryBarrier.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	ImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	ImageMemoryBarrier.subresourceRange.layerCount = 1;
	ImageMemoryBarrier.subresourceRange.levelCount = renderTextureCreateInfo.MIPLevels;

	vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 1, &ImageMemoryBarrier);

	VkBufferImageCopy BufferImageCopies[16];

	for (uint32_t i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
	{
		BufferImageCopies[i].bufferImageHeight = renderTextureCreateInfo.Height >> i;
		BufferImageCopies[i].bufferOffset = (i == 0) ? 0 : BufferImageCopies[i - 1].bufferOffset + 4 * (BufferImageCopies[i - 1].imageExtent.width * BufferImageCopies[i - 1].imageExtent.height);
		BufferImageCopies[i].bufferRowLength = renderTextureCreateInfo.Width >> i;
		BufferImageCopies[i].imageExtent.depth = 1;
		BufferImageCopies[i].imageExtent.height = renderTextureCreateInfo.Height >> i;
		BufferImageCopies[i].imageExtent.width = renderTextureCreateInfo.Width >> i;
		BufferImageCopies[i].imageOffset.x = 0;
		BufferImageCopies[i].imageOffset.y = 0;
		BufferImageCopies[i].imageOffset.z = 0;
		BufferImageCopies[i].imageSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		BufferImageCopies[i].imageSubresource.baseArrayLayer = 0;
		BufferImageCopies[i].imageSubresource.layerCount = 1;
		BufferImageCopies[i].imageSubresource.mipLevel = i;
	}

	vkCmdCopyBufferToImage(CommandBuffers[0], UploadBuffer, renderTexture->Texture, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, renderTextureCreateInfo.MIPLevels, BufferImageCopies);

	ImageMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
	ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrier.image = renderTexture->Texture;
	ImageMemoryBarrier.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ImageMemoryBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	ImageMemoryBarrier.pNext = nullptr;
	ImageMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
	ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImageMemoryBarrier.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	ImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	ImageMemoryBarrier.subresourceRange.layerCount = 1;
	ImageMemoryBarrier.subresourceRange.levelCount = renderTextureCreateInfo.MIPLevels;

	vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrier);

	Result = vkEndCommandBuffer(CommandBuffers[0]);

	VkSubmitInfo SubmitInfo;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffers[0];
	SubmitInfo.pNext = nullptr;
	SubmitInfo.pSignalSemaphores = nullptr;
	SubmitInfo.pWaitDstStageMask = nullptr;
	SubmitInfo.pWaitSemaphores = nullptr;
	SubmitInfo.signalSemaphoreCount = 0;
	SubmitInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.waitSemaphoreCount = 0;

	Result = vkQueueSubmit(CommandQueue, 1, &SubmitInfo, VK_NULL_HANDLE);

	Result = vkQueueWaitIdle(CommandQueue);

	VkImageViewCreateInfo ImageViewCreateInfo;
	ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.flags = 0;
	ImageViewCreateInfo.format = renderTextureCreateInfo.SRGB ? VkFormat::VK_FORMAT_R8G8B8A8_SRGB : VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	ImageViewCreateInfo.image = renderTexture->Texture;
	ImageViewCreateInfo.pNext = nullptr;
	ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ImageViewCreateInfo.subresourceRange.layerCount = 1;
	ImageViewCreateInfo.subresourceRange.levelCount = renderTextureCreateInfo.MIPLevels;
	ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

	Result = vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &renderTexture->TextureView);

	return renderTexture;
}

RenderMaterial* RenderSystem::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterial *renderMaterial = new RenderMaterial();

	VkResult Result;

	VkShaderModule VertexShaderModule, PixelShaderModule;

	VkShaderModuleCreateInfo ShaderModuleCreateInfo;
	ShaderModuleCreateInfo.codeSize = renderMaterialCreateInfo.VertexShaderByteCodeLength;
	ShaderModuleCreateInfo.flags = 0;
	ShaderModuleCreateInfo.pCode = (uint32_t*)renderMaterialCreateInfo.VertexShaderByteCodeData;
	ShaderModuleCreateInfo.pNext = nullptr;
	ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	Result = vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &VertexShaderModule);

	ShaderModuleCreateInfo.codeSize = renderMaterialCreateInfo.PixelShaderByteCodeLength;
	ShaderModuleCreateInfo.flags = 0;
	ShaderModuleCreateInfo.pCode = (uint32_t*)renderMaterialCreateInfo.PixelShaderByteCodeData;
	ShaderModuleCreateInfo.pNext = nullptr;
	ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	Result = vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &PixelShaderModule);

	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState;
	ZeroMemory(&PipelineColorBlendAttachmentState, sizeof(PipelineColorBlendAttachmentState));
	PipelineColorBlendAttachmentState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo;
	ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
	PipelineColorBlendStateCreateInfo.attachmentCount = 1;
	PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
	PipelineColorBlendStateCreateInfo.pNext = nullptr;
	PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo;
	ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
	PipelineDepthStencilStateCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;
	PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	PipelineDepthStencilStateCreateInfo.pNext = nullptr;
	PipelineDepthStencilStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	VkDynamicState DynamicStates[4] =
	{
		VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
		VkDynamicState::VK_DYNAMIC_STATE_SCISSOR,
		VkDynamicState::VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		VkDynamicState::VK_DYNAMIC_STATE_BLEND_CONSTANTS
	};

	VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo;
	PipelineDynamicStateCreateInfo.dynamicStateCount = 4;
	PipelineDynamicStateCreateInfo.flags = 0;
	PipelineDynamicStateCreateInfo.pDynamicStates = DynamicStates;
	PipelineDynamicStateCreateInfo.pNext = nullptr;
	PipelineDynamicStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo;
	PipelineInputAssemblyStateCreateInfo.flags = 0;
	PipelineInputAssemblyStateCreateInfo.pNext = nullptr;
	PipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	PipelineInputAssemblyStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	PipelineInputAssemblyStateCreateInfo.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkSampleMask SampleMask = 0xFFFFFFFF;

	VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo;
	PipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.flags = 0;
	PipelineMultisampleStateCreateInfo.minSampleShading = 0.0f;
	PipelineMultisampleStateCreateInfo.pNext = nullptr;
	PipelineMultisampleStateCreateInfo.pSampleMask = &SampleMask;
	PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	PipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo;
	ZeroMemory(&PipelineRasterizationStateCreateInfo, sizeof(VkPipelineRasterizationStateCreateInfo));
	PipelineRasterizationStateCreateInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
	PipelineRasterizationStateCreateInfo.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
	PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	PipelineRasterizationStateCreateInfo.pNext = nullptr;
	PipelineRasterizationStateCreateInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
	PipelineRasterizationStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfos[2];
	PipelineShaderStageCreateInfos[0].flags = 0;
	PipelineShaderStageCreateInfos[0].module = VertexShaderModule;
	PipelineShaderStageCreateInfos[0].pName = "VS";
	PipelineShaderStageCreateInfos[0].pNext = nullptr;
	PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
	PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	PipelineShaderStageCreateInfos[1].flags = 0;
	PipelineShaderStageCreateInfos[1].module = PixelShaderModule;
	PipelineShaderStageCreateInfos[1].pName = "PS";
	PipelineShaderStageCreateInfos[1].pNext = nullptr;
	PipelineShaderStageCreateInfos[1].pSpecializationInfo = nullptr;
	PipelineShaderStageCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	PipelineShaderStageCreateInfos[1].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	VkVertexInputAttributeDescription VertexInputAttributeDescriptions[2];
	VertexInputAttributeDescriptions[0].binding = 0;
	VertexInputAttributeDescriptions[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[0].location = 0;
	VertexInputAttributeDescriptions[0].offset = 0;
	VertexInputAttributeDescriptions[1].binding = 0;
	VertexInputAttributeDescriptions[1].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
	VertexInputAttributeDescriptions[1].location = 1;
	VertexInputAttributeDescriptions[1].offset = 12;

	VkVertexInputBindingDescription VertexInputBindingDescription;
	VertexInputBindingDescription.binding = 0;
	VertexInputBindingDescription.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
	VertexInputBindingDescription.stride = 20;

	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;
	PipelineVertexInputStateCreateInfo.flags = 0;
	PipelineVertexInputStateCreateInfo.pNext = nullptr;
	PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttributeDescriptions;
	PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &VertexInputBindingDescription;
	PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
	PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;

	VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo;
	PipelineViewportStateCreateInfo.flags = 0;
	PipelineViewportStateCreateInfo.pNext = nullptr;
	PipelineViewportStateCreateInfo.pScissors = nullptr;
	PipelineViewportStateCreateInfo.pViewports = nullptr;
	PipelineViewportStateCreateInfo.scissorCount = 1;
	PipelineViewportStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	PipelineViewportStateCreateInfo.viewportCount = 1;

	VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo;
	GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	GraphicsPipelineCreateInfo.basePipelineIndex = -1;
	GraphicsPipelineCreateInfo.flags = 0;
	GraphicsPipelineCreateInfo.layout = PipelineLayout;
	GraphicsPipelineCreateInfo.pColorBlendState = &PipelineColorBlendStateCreateInfo;
	GraphicsPipelineCreateInfo.pDepthStencilState = &PipelineDepthStencilStateCreateInfo;
	GraphicsPipelineCreateInfo.pDynamicState = &PipelineDynamicStateCreateInfo;
	GraphicsPipelineCreateInfo.pInputAssemblyState = &PipelineInputAssemblyStateCreateInfo;
	GraphicsPipelineCreateInfo.pMultisampleState = &PipelineMultisampleStateCreateInfo;
	GraphicsPipelineCreateInfo.pNext = nullptr;
	GraphicsPipelineCreateInfo.pRasterizationState = &PipelineRasterizationStateCreateInfo;
	GraphicsPipelineCreateInfo.pStages = PipelineShaderStageCreateInfos;
	GraphicsPipelineCreateInfo.pTessellationState = nullptr;
	GraphicsPipelineCreateInfo.pVertexInputState = &PipelineVertexInputStateCreateInfo;
	GraphicsPipelineCreateInfo.pViewportState = &PipelineViewportStateCreateInfo;
	GraphicsPipelineCreateInfo.renderPass = RenderPass;
	GraphicsPipelineCreateInfo.stageCount = 2;
	GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	GraphicsPipelineCreateInfo.subpass = 0;

	Result = vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &renderMaterial->Pipeline);

	vkDestroyShaderModule(Device, VertexShaderModule, nullptr);
	vkDestroyShaderModule(Device, PixelShaderModule, nullptr);

	return renderMaterial;
}

void RenderSystem::DestroyRenderMesh(RenderMesh* renderMesh)
{
	RenderMeshDestructionQueue.push_back(renderMesh);
}

void RenderSystem::DestroyRenderTexture(RenderTexture* renderTexture)
{
	RenderTextureDestructionQueue.push_back(renderTexture);
}

void RenderSystem::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	RenderMaterialDestructionQueue.push_back(renderMaterial);
}

void RenderSystem::CheckDXCallResult(HRESULT hr, const wchar_t* Function)
{
	if (FAILED(hr))
	{
		wchar_t DXErrorMessageBuffer[2048];
		wchar_t DXErrorCodeBuffer[512];

		const wchar_t *DXErrorCodePtr = GetDXErrorMessageFromHRESULT(hr);

		if (DXErrorCodePtr) wcscpy(DXErrorCodeBuffer, DXErrorCodePtr);
		else wsprintf(DXErrorCodeBuffer, L"0x%08X (неизвестный код)", hr);

		wsprintf(DXErrorMessageBuffer, L"Произошла ошибка при попытке вызова следующей DirectX-функции:\r\n%s\r\nКод ошибки: %s", Function, DXErrorCodeBuffer);

		int IntResult = MessageBox(NULL, DXErrorMessageBuffer, L"Ошибка DirectX", MB_OK | MB_ICONERROR);

		if (hr == DXGI_ERROR_DEVICE_REMOVED) SAFE_DX(Device->GetDeviceRemovedReason());

		ExitProcess(0);
	}
}

const wchar_t* RenderSystem::GetDXErrorMessageFromHRESULT(HRESULT hr)
{
	switch (hr)
	{
		case E_UNEXPECTED:
			return L"E_UNEXPECTED"; 
			break; 
		case E_NOTIMPL:
			return L"E_NOTIMPL";
			break;
		case E_OUTOFMEMORY:
			return L"E_OUTOFMEMORY";
			break; 
		case E_INVALIDARG:
			return L"E_INVALIDARG";
			break; 
		case E_NOINTERFACE:
			return L"E_NOINTERFACE";
			break; 
		case E_POINTER:
			return L"E_POINTER"; 
			break; 
		case E_HANDLE:
			return L"E_HANDLE"; 
			break;
		case E_ABORT:
			return L"E_ABORT"; 
			break; 
		case E_FAIL:
			return L"E_FAIL"; 
			break;
		case E_ACCESSDENIED:
			return L"E_ACCESSDENIED";
			break; 
		case E_PENDING:
			return L"E_PENDING";
			break; 
		case E_BOUNDS:
			return L"E_BOUNDS";
			break; 
		case E_CHANGED_STATE:
			return L"E_CHANGED_STATE";
			break; 
		case E_ILLEGAL_STATE_CHANGE:
			return L"E_ILLEGAL_STATE_CHANGE";
			break; 
		case E_ILLEGAL_METHOD_CALL:
			return L"E_ILLEGAL_METHOD_CALL";
			break; 
		case E_STRING_NOT_NULL_TERMINATED:
			return L"E_STRING_NOT_NULL_TERMINATED"; 
			break; 
		case E_ILLEGAL_DELEGATE_ASSIGNMENT:
			return L"E_ILLEGAL_DELEGATE_ASSIGNMENT";
			break; 
		case E_ASYNC_OPERATION_NOT_STARTED:
			return L"E_ASYNC_OPERATION_NOT_STARTED"; 
			break; 
		case E_APPLICATION_EXITING:
			return L"E_APPLICATION_EXITING";
			break; 
		case E_APPLICATION_VIEW_EXITING:
			return L"E_APPLICATION_VIEW_EXITING";
			break; 
		case DXGI_ERROR_INVALID_CALL:
			return L"DXGI_ERROR_INVALID_CALL";
			break; 
		case DXGI_ERROR_NOT_FOUND:
			return L"DXGI_ERROR_NOT_FOUND";
			break; 
		case DXGI_ERROR_MORE_DATA:
			return L"DXGI_ERROR_MORE_DATA";
			break; 
		case DXGI_ERROR_UNSUPPORTED:
			return L"DXGI_ERROR_UNSUPPORTED";
			break; 
		case DXGI_ERROR_DEVICE_REMOVED:
			return L"DXGI_ERROR_DEVICE_REMOVED";
			break; 
		case DXGI_ERROR_DEVICE_HUNG:
			return L"DXGI_ERROR_DEVICE_HUNG";
			break; 
		case DXGI_ERROR_DEVICE_RESET:
			return L"DXGI_ERROR_DEVICE_RESET";
			break; 
		case DXGI_ERROR_WAS_STILL_DRAWING:
			return L"DXGI_ERROR_WAS_STILL_DRAWING";
			break; 
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
			return L"DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
			break; 
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE:
			return L"DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE"; 
			break; 
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			return L"DXGI_ERROR_DRIVER_INTERNAL_ERROR";
			break; 
		case DXGI_ERROR_NONEXCLUSIVE:
			return L"DXGI_ERROR_NONEXCLUSIVE";
			break; 
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			return L"DXGI_ERROR_NOT_CURRENTLY_AVAILABLE"; 
			break; 
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:
			return L"DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
			break; 
		case DXGI_ERROR_REMOTE_OUTOFMEMORY:
			return L"DXGI_ERROR_REMOTE_OUTOFMEMORY"; 
			break; 
		case DXGI_ERROR_ACCESS_LOST:
			return L"DXGI_ERROR_ACCESS_LOST";
			break; 
		case DXGI_ERROR_WAIT_TIMEOUT:
			return L"DXGI_ERROR_WAIT_TIMEOUT";
			break; 
		case DXGI_ERROR_SESSION_DISCONNECTED:
			return L"DXGI_ERROR_SESSION_DISCONNECTED";
			break; 
		case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE:
			return L"DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";
			break; 
		case DXGI_ERROR_CANNOT_PROTECT_CONTENT:
			return L"DXGI_ERROR_CANNOT_PROTECT_CONTENT";
			break; 
		case DXGI_ERROR_ACCESS_DENIED:
			return L"DXGI_ERROR_ACCESS_DENIED"; 
			break; 
		case DXGI_ERROR_NAME_ALREADY_EXISTS:
			return L"DXGI_ERROR_NAME_ALREADY_EXISTS";
			break; 
		case DXGI_ERROR_SDK_COMPONENT_MISSING:
			return L"DXGI_ERROR_SDK_COMPONENT_MISSING"; 
			break; 
		case DXGI_ERROR_NOT_CURRENT:
			return L"DXGI_ERROR_NOT_CURRENT";
			break; 
		case DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY:
			return L"DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY"; 
			break; 
		case DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION:
			return L"DXGI_ERROR_DYNAMIC_CODE_POLICY_VIOLATION"; 
			break; 
		case DXGI_ERROR_NON_COMPOSITED_UI:
			return L"DXGI_ERROR_NON_COMPOSITED_UI";
			break; 
		case DXGI_ERROR_MODE_CHANGE_IN_PROGRESS:
			return L"DXGI_ERROR_MODE_CHANGE_IN_PROGRESS";
			break; 
		case DXGI_ERROR_CACHE_CORRUPT:
			return L"DXGI_ERROR_CACHE_CORRUPT";
			break;
		case DXGI_ERROR_CACHE_FULL:
			return L"DXGI_ERROR_CACHE_FULL";
			break;
		case DXGI_ERROR_CACHE_HASH_COLLISION:
			return L"DXGI_ERROR_CACHE_HASH_COLLISION";
			break;
		case DXGI_ERROR_ALREADY_EXISTS:
			return L"DXGI_ERROR_ALREADY_EXISTS"; 
			break;
		case D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return L"D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break; 
		case D3D10_ERROR_FILE_NOT_FOUND:
			return L"D3D10_ERROR_FILE_NOT_FOUND";
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS:
			return L"D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
			break;
		case D3D11_ERROR_FILE_NOT_FOUND:
			return L"D3D11_ERROR_FILE_NOT_FOUND"; 
			break;
		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS:
			return L"D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS"; 
			break; 
		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD:
			return L"D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
			break;
		case D3D12_ERROR_ADAPTER_NOT_FOUND:
			return L"D3D12_ERROR_ADAPTER_NOT_FOUND";
			break;
		case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
			return L"D3D12_ERROR_DRIVER_VERSION_MISMATCH"; 
			break;
		default:
			return nullptr;
			break;
	}

	return nullptr;
}