// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

struct PointLight
{
	XMFLOAT3 Position;
	float Radius;
	XMFLOAT3 Color;
	float Brightness;
};

struct GBufferOpaquePassConstantBuffer
{
	XMMATRIX WVPMatrix;
	XMMATRIX WorldMatrix;
	XMFLOAT3X4 VectorTransformMatrix;
};

struct ShadowMapPassConstantBuffer
{
	XMMATRIX WVPMatrix;
};

struct ShadowResolveConstantBuffer
{
	XMMATRIX ReProjMatrices[4];
};

struct DeferredLightingConstantBuffer
{
	XMMATRIX InvViewProjMatrix;
	XMFLOAT3 CameraWorldPosition;
};

struct SkyConstantBuffer
{
	XMMATRIX WVPMatrix;
};

struct SunConstantBuffer
{
	XMMATRIX ViewMatrix;
	XMMATRIX ProjMatrix;
	XMFLOAT3 SunPosition;
};

#ifdef _DEBUG
VkBool32 VKAPI_PTR vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,	VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) OutputDebugString((const wchar_t*)u"[ERROR]");
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) OutputDebugString((const wchar_t*)u"[INFO]");
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) OutputDebugString((const wchar_t*)u"[VERBOSE]");
	if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) OutputDebugString((const wchar_t*)u"[WARNING]");

	OutputDebugString((const wchar_t*)u" ");

	if (messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) OutputDebugString((const wchar_t*)u"[GENERAL]");
	if (messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) OutputDebugString((const wchar_t*)u"[PERFORMANCE]");
	if (messageTypes & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) OutputDebugString((const wchar_t*)u"[VALIDATION]");

	OutputDebugString((const wchar_t*)u" ");

	char16_t *Message = new char16_t[strlen(pCallbackData->pMessage) + 1];

	for (size_t i = 0; pCallbackData->pMessage[i] != 0; i++)
	{
		Message[i] = pCallbackData->pMessage[i];
	}

	Message[strlen(pCallbackData->pMessage)] = 0;

	OutputDebugString((const wchar_t*)Message);

	delete[] Message;

	OutputDebugString(L"\r\n");

	return VK_FALSE;
}
#endif

void RenderSystem::InitSystem()
{
	uint32_t APIVersion;

	SAFE_VK(vkEnumerateInstanceVersion(&APIVersion));

	uint32_t InstanceLayerPropertiesCount;
	SAFE_VK(vkEnumerateInstanceLayerProperties(&InstanceLayerPropertiesCount, nullptr));
	VkLayerProperties *InstanceLayerProperties = new VkLayerProperties[InstanceLayerPropertiesCount];
	SAFE_VK(vkEnumerateInstanceLayerProperties(&InstanceLayerPropertiesCount, InstanceLayerProperties));

	uint32_t InstanceExtensionPropertiesCount;
	SAFE_VK(vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionPropertiesCount, nullptr));
	VkExtensionProperties *InstanceExtensionProperties = new VkExtensionProperties[InstanceExtensionPropertiesCount];
	SAFE_VK(vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionPropertiesCount, InstanceExtensionProperties));
	delete[] InstanceExtensionProperties;

	for (uint32_t i = 0; i < InstanceLayerPropertiesCount; i++)
	{
		SAFE_VK(vkEnumerateInstanceExtensionProperties(InstanceLayerProperties[i].layerName, &InstanceExtensionPropertiesCount, nullptr));
		VkExtensionProperties *InstanceExtensionProperties = new VkExtensionProperties[InstanceExtensionPropertiesCount];
		SAFE_VK(vkEnumerateInstanceExtensionProperties(InstanceLayerProperties[i].layerName, &InstanceExtensionPropertiesCount, InstanceExtensionProperties));
		delete[] InstanceExtensionProperties;
	}

	delete[] InstanceLayerProperties;

	VkApplicationInfo ApplicationInfo;
	ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	ApplicationInfo.pApplicationName = "Tolerance Paradox";
	ApplicationInfo.pEngineName = "Paradox Engine";
	ApplicationInfo.pNext = nullptr;
	ApplicationInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;

#ifdef _DEBUG
	uint32_t EnabledInstanceExtensionsCount = 4;
	uint32_t EnabledInstanceLayersCount = 1;

	const char* EnabledInstanceExtensionsNames[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
	const char* EnabledInstanceLayersNames[] = { "VK_LAYER_KHRONOS_validation" };
#else
	uint32_t EnabledInstanceExtensionsCount = 3;
	uint32_t EnabledInstanceLayersCount = 0;

	const char* EnabledInstanceExtensionsNames[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
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
	
	SAFE_VK(vkCreateInstance(&InstanceCreateInfo, nullptr, &Instance));

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

	SAFE_VK(vkCreateDebugUtilsMessengerEXT(Instance, &DebugUtilsMessengerCreateInfo, nullptr, &DebugUtilsMessenger));
#endif

	uint32_t PhysicalDevicesCount;
	SAFE_VK(vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, nullptr));
	VkPhysicalDevice *PhysicalDevices = new VkPhysicalDevice[PhysicalDevicesCount];
	SAFE_VK(vkEnumeratePhysicalDevices(Instance, &PhysicalDevicesCount, PhysicalDevices));

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
	SAFE_VK(vkEnumerateDeviceLayerProperties(PhysicalDevice, &DeviceLayerPropertiesCount, nullptr));
	VkLayerProperties *DeviceLayerProperties = new VkLayerProperties[DeviceLayerPropertiesCount];
	SAFE_VK(vkEnumerateDeviceLayerProperties(PhysicalDevice, &DeviceLayerPropertiesCount, DeviceLayerProperties));

	uint32_t DeviceExtensionPropertiesCount;
	SAFE_VK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionPropertiesCount, nullptr));
	VkExtensionProperties *DeviceExtensionProperties = new VkExtensionProperties[DeviceExtensionPropertiesCount];
	SAFE_VK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionPropertiesCount, DeviceExtensionProperties));
	delete[] DeviceExtensionProperties;

	for (uint32_t i = 0; i < DeviceLayerPropertiesCount; i++)
	{
		SAFE_VK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, DeviceLayerProperties[i].layerName, &DeviceExtensionPropertiesCount, nullptr));
		VkExtensionProperties *DeviceExtensionProperties = new VkExtensionProperties[DeviceExtensionPropertiesCount];
		SAFE_VK(vkEnumerateDeviceExtensionProperties(PhysicalDevice, DeviceLayerProperties[i].layerName, &DeviceExtensionPropertiesCount, DeviceExtensionProperties));
		delete[] DeviceExtensionProperties;
	}

	delete[] DeviceLayerProperties;

	VkPhysicalDeviceFeatures AvailablePhysicalDeviceFeatures, EnabledPhysicalDeviceFeatures;

	vkGetPhysicalDeviceFeatures(PhysicalDevice, &AvailablePhysicalDeviceFeatures);

	ZeroMemory(&EnabledPhysicalDeviceFeatures, sizeof(VkPhysicalDeviceFeatures));
	EnabledPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
	EnabledPhysicalDeviceFeatures.shaderImageGatherExtended = VK_TRUE;

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

	uint32_t EnabledDeviceExtensionsCount = 5;

	const char* EnabledDeviceExtensionsNames[] = 
	{ 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MULTIVIEW_EXTENSION_NAME, 
		VK_KHR_MAINTENANCE2_EXTENSION_NAME, 
		VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, 
		VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME 
	};

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
	
	SAFE_VK(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device));

	VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;

	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalDeviceMemoryProperties);

	VkWin32SurfaceCreateInfoKHR Win32SurfaceCreateInfo;
	Win32SurfaceCreateInfo.flags = 0;
	Win32SurfaceCreateInfo.hinstance = GetModuleHandle(NULL);
	Win32SurfaceCreateInfo.hwnd = Application::GetMainWindowHandle();
	Win32SurfaceCreateInfo.pNext = nullptr;
	Win32SurfaceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;

	SAFE_VK(vkCreateWin32SurfaceKHR(Instance, &Win32SurfaceCreateInfo, nullptr, &Surface));

	VkSurfaceCapabilitiesKHR SurfaceCapabilities;

	SAFE_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &SurfaceCapabilities));

	uint32_t SurfaceFormatsCount;
	SAFE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatsCount, nullptr));
	VkSurfaceFormatKHR *SurfaceFormats = new VkSurfaceFormatKHR[SurfaceFormatsCount];
	SAFE_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &SurfaceFormatsCount, SurfaceFormats));

	uint32_t PresentModesCount;
	SAFE_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModesCount, nullptr));
	VkPresentModeKHR *PresentModes = new VkPresentModeKHR[PresentModesCount];
	SAFE_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModesCount, PresentModes));

	VkBool32 SurfaceSupport;
	SAFE_VK(vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, QueueFamilyIndex, Surface, &SurfaceSupport));

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

	SAFE_VK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, nullptr, &SwapChain));

	vkGetDeviceQueue(Device, QueueFamilyIndex, 0, &CommandQueue);

	VkCommandPoolCreateInfo CommandPoolCreateInfo;
	CommandPoolCreateInfo.flags = VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	CommandPoolCreateInfo.pNext = nullptr;
	CommandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
	CommandPoolCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	SAFE_VK(vkCreateCommandPool(Device, &CommandPoolCreateInfo, nullptr, &CommandPool));

	VkCommandBufferAllocateInfo CommandBufferAllocateInfo;
	CommandBufferAllocateInfo.commandBufferCount = 2;
	CommandBufferAllocateInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CommandBufferAllocateInfo.pNext = nullptr;
	CommandBufferAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	CommandBufferAllocateInfo.commandPool = CommandPool;
	SAFE_VK(vkAllocateCommandBuffers(Device, &CommandBufferAllocateInfo, CommandBuffers));

	CurrentFrameIndex = 0;

	VkSemaphoreCreateInfo SemaphoreCreateInfo;
	SemaphoreCreateInfo.flags = 0;
	SemaphoreCreateInfo.pNext = nullptr;
	SemaphoreCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	SAFE_VK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ImageAvailabilitySemaphore));
	SAFE_VK(vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &ImagePresentationSemaphore));

	VkFenceCreateInfo FenceCreateInfo;
	FenceCreateInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;
	FenceCreateInfo.pNext = nullptr;
	FenceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	
	SAFE_VK(vkCreateFence(Device, &FenceCreateInfo, nullptr, &FrameSyncFences[0]));
	SAFE_VK(vkCreateFence(Device, &FenceCreateInfo, nullptr, &FrameSyncFences[1]));

	FenceCreateInfo.flags = 0;

	SAFE_VK(vkCreateFence(Device, &FenceCreateInfo, nullptr, &CopySyncFence));

	VkDescriptorPoolSize DescriptorPoolSizes[3];
	DescriptorPoolSizes[0].descriptorCount = 100000 + 4;
	DescriptorPoolSizes[0].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescriptorPoolSizes[1].descriptorCount = 40000 + 1 + 5;
	DescriptorPoolSizes[1].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorPoolSizes[2].descriptorCount = 1 + 1 + 1;
	DescriptorPoolSizes[2].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;

	VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
	DescriptorPoolCreateInfo.flags = 0;
	DescriptorPoolCreateInfo.maxSets = 100000 + 20000 + 1 + 1 + 1;
	DescriptorPoolCreateInfo.pNext = nullptr;
	DescriptorPoolCreateInfo.poolSizeCount = 3;
	DescriptorPoolCreateInfo.pPoolSizes = DescriptorPoolSizes;
	DescriptorPoolCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

	SAFE_VK(vkCreateDescriptorPool(Device, &DescriptorPoolCreateInfo, nullptr, &DescriptorPools[0]));
	SAFE_VK(vkCreateDescriptorPool(Device, &DescriptorPoolCreateInfo, nullptr, &DescriptorPools[1]));

	VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[2];
	DescriptorSetLayoutBindings[0].binding = 0;
	DescriptorSetLayoutBindings[0].descriptorCount = 1;
	DescriptorSetLayoutBindings[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
	DescriptorSetLayoutBindings[0].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
	DescriptorSetLayoutCreateInfo.bindingCount = 1;
	DescriptorSetLayoutCreateInfo.flags = 0;
	DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
	DescriptorSetLayoutCreateInfo.pNext = nullptr;
	DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	
	SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &ConstantBuffersSetLayout));

	DescriptorSetLayoutBindings[0].binding = 0;
	DescriptorSetLayoutBindings[0].descriptorCount = 1;
	DescriptorSetLayoutBindings[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
	DescriptorSetLayoutBindings[0].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	DescriptorSetLayoutBindings[1].binding = 1;
	DescriptorSetLayoutBindings[1].descriptorCount = 1;
	DescriptorSetLayoutBindings[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;
	DescriptorSetLayoutBindings[1].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

	DescriptorSetLayoutCreateInfo.bindingCount = 2;
	SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &TexturesSetLayout));

	DescriptorSetLayoutBindings[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
	DescriptorSetLayoutBindings[0].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

	DescriptorSetLayoutCreateInfo.bindingCount = 1;
	SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &SamplersSetLayout));

	VkDescriptorSetLayout DescriptorSetLayouts[3] = { ConstantBuffersSetLayout, TexturesSetLayout, SamplersSetLayout };

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
	PipelineLayoutCreateInfo.flags = 0;
	PipelineLayoutCreateInfo.pNext = nullptr;
	PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	PipelineLayoutCreateInfo.pSetLayouts = DescriptorSetLayouts;
	PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	PipelineLayoutCreateInfo.setLayoutCount = 3;
	PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));

	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
	DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
	DescriptorSetAllocateInfo.descriptorSetCount = 1;
	DescriptorSetAllocateInfo.pNext = nullptr;
	DescriptorSetAllocateInfo.pSetLayouts = &SamplersSetLayout;
	DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SamplersSets[0]));

	DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];

	SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SamplersSets[1]));

	for (uint32_t i = 0; i < 20000; i++)
	{
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		DescriptorSetAllocateInfo.pSetLayouts = &ConstantBuffersSetLayout;
		
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[0][i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[0][20000 + i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[0][2 * 20000 + i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[0][3 * 20000 + i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[0][4 * 20000 + i]));

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];

		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[1][i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[1][20000 + i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[1][2 * 20000 + i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[1][3 * 20000 + i]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ConstantBuffersSets[1][4 * 20000 + i]));

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		DescriptorSetAllocateInfo.pSetLayouts = &TexturesSetLayout;

		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &TexturesSets[0][i]));

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];

		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &TexturesSets[1][i]));
	}

	HANDLE FullScreenQuadVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/FullScreenQuad.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER FullScreenQuadVertexShaderByteCodeLength;
	BOOL Result = GetFileSizeEx(FullScreenQuadVertexShaderFile, &FullScreenQuadVertexShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> FullScreenQuadVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FullScreenQuadVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(FullScreenQuadVertexShaderFile, FullScreenQuadVertexShaderByteCodeData, (DWORD)FullScreenQuadVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(FullScreenQuadVertexShaderFile);

	VkShaderModule FullScreenQuadShaderModule;

	VkShaderModuleCreateInfo ShaderModuleCreateInfo;
	ShaderModuleCreateInfo.codeSize = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	ShaderModuleCreateInfo.flags = 0;
	ShaderModuleCreateInfo.pCode = FullScreenQuadVertexShaderByteCodeData;
	ShaderModuleCreateInfo.pNext = nullptr;
	ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &FullScreenQuadShaderModule));

	// ===============================================================================================================

	{
		SAFE_VK(vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImagesCount, nullptr));
		BackBufferTextures = new VkImage[SwapChainImagesCount];
		SAFE_VK(vkGetSwapchainImagesKHR(Device, SwapChain, &SwapChainImagesCount, BackBufferTextures));

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

		BackBufferTexturesViews = new VkImageView[SwapChainImagesCount];

		for (uint32_t i = 0; i < SwapChainImagesCount; i++)
		{
			ImageViewCreateInfo.image = BackBufferTextures[i];
			SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &BackBufferTexturesViews[i]));
		}
	}

	// ===============================================================================================================

	{
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

		SAFE_VK(vkCreateSampler(Device, &SamplerCreateInfo, nullptr, &TextureSampler));

		VkSamplerReductionModeCreateInfo SamplerReductionModeCreateInfo;
		SamplerReductionModeCreateInfo.pNext = nullptr;
		SamplerReductionModeCreateInfo.reductionMode = VkSamplerReductionMode::VK_SAMPLER_REDUCTION_MODE_MIN;
		SamplerReductionModeCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;

		SamplerCreateInfo.addressModeU = SamplerCreateInfo.addressModeV = SamplerCreateInfo.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		SamplerCreateInfo.anisotropyEnable = VK_FALSE;
		SamplerCreateInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		SamplerCreateInfo.compareEnable = VK_FALSE;
		SamplerCreateInfo.compareOp = (VkCompareOp)0;
		SamplerCreateInfo.flags = 0;
		SamplerCreateInfo.magFilter = VkFilter::VK_FILTER_LINEAR;
		SamplerCreateInfo.maxAnisotropy = 1.0f;
		SamplerCreateInfo.maxLod = FLT_MAX;
		SamplerCreateInfo.minFilter = VkFilter::VK_FILTER_LINEAR;
		SamplerCreateInfo.minLod = 0.0f;
		SamplerCreateInfo.mipLodBias = 0.0f;
		SamplerCreateInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		SamplerCreateInfo.pNext = &SamplerReductionModeCreateInfo;
		SamplerCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		SAFE_VK(vkCreateSampler(Device, &SamplerCreateInfo, nullptr, &MinSampler));

		SamplerCreateInfo.addressModeU = SamplerCreateInfo.addressModeV = SamplerCreateInfo.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		SamplerCreateInfo.anisotropyEnable = VK_FALSE;
		SamplerCreateInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		SamplerCreateInfo.compareEnable = VK_TRUE;
		SamplerCreateInfo.compareOp = VkCompareOp::VK_COMPARE_OP_LESS;
		SamplerCreateInfo.flags = 0;
		SamplerCreateInfo.magFilter = VkFilter::VK_FILTER_LINEAR;
		SamplerCreateInfo.maxAnisotropy = 1.0f;
		SamplerCreateInfo.maxLod = FLT_MAX;
		SamplerCreateInfo.minFilter = VkFilter::VK_FILTER_LINEAR;
		SamplerCreateInfo.minLod = 0.0f;
		SamplerCreateInfo.mipLodBias = 0.0f;
		SamplerCreateInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
		SamplerCreateInfo.pNext = nullptr;
		SamplerCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		SAFE_VK(vkCreateSampler(Device, &SamplerCreateInfo, nullptr, &ShadowMapSampler));
	}

	// ===============================================================================================================

	{
		VkMemoryAllocateInfo MemoryAllocateInfo;
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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &TextureMemoryHeaps[CurrentTextureMemoryHeapIndex]));

		VkBufferCreateInfo BufferCreateInfo;
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = UPLOAD_HEAP_SIZE;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &UploadBuffer));

		VkMemoryRequirements MemoryRequirements;

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &UploadHeap));

		SAFE_VK(vkBindBufferMemory(Device, UploadBuffer, UploadHeap, 0));
	}

	// ===============================================================================================================

	{
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
		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &GBufferTextures[0]));
		ImageCreateInfo.format = VkFormat::VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &GBufferTextures[1]));

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, GBufferTextures[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GBufferTexturesMemoryHeaps[0]));

		SAFE_VK(vkBindImageMemory(Device, GBufferTextures[0], GBufferTexturesMemoryHeaps[0], 0));

		vkGetImageMemoryRequirements(Device, GBufferTextures[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GBufferTexturesMemoryHeaps[1]));

		SAFE_VK(vkBindImageMemory(Device, GBufferTextures[1], GBufferTexturesMemoryHeaps[1], 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		//ImageViewCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		//ImageViewCreateInfo.image = DepthBufferTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
		ImageViewCreateInfo.image = GBufferTextures[0];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &GBufferTexturesViews[0]));
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		ImageViewCreateInfo.image = GBufferTextures[1];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &GBufferTexturesViews[1]));

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

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &DepthBufferTexture));

		vkGetImageMemoryRequirements(Device, DepthBufferTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &DepthBufferTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, DepthBufferTexture, DepthBufferTextureMemoryHeap, 0));

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

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &DepthBufferTextureView));

		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		ImageViewCreateInfo.image = DepthBufferTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &DepthBufferTextureDepthReadView));

		VkAttachmentDescription AttachmentDescriptions[3];
		AttachmentDescriptions[0].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].flags = 0;
		AttachmentDescriptions[0].format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
		AttachmentDescriptions[0].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescriptions[0].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[0].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[0].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[0].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[1].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].flags = 0;
		AttachmentDescriptions[1].format = VkFormat::VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		AttachmentDescriptions[1].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescriptions[1].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[1].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[1].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[1].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[2].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[2].flags = 0;
		AttachmentDescriptions[2].format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		AttachmentDescriptions[2].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[2].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescriptions[2].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[2].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescriptions[2].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[2].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference AttachmentReferences[3];
		AttachmentReferences[0].attachment = 0;
		AttachmentReferences[0].layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentReferences[1].attachment = 1;
		AttachmentReferences[1].layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentReferences[2].attachment = 2;
		AttachmentReferences[2].layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription SubpassDescription;
		SubpassDescription.colorAttachmentCount = 2;
		SubpassDescription.flags = 0;
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = &AttachmentReferences[0];
		SubpassDescription.pDepthStencilAttachment = &AttachmentReferences[2];
		SubpassDescription.pInputAttachments = nullptr;
		SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.pPreserveAttachments = nullptr;
		SubpassDescription.preserveAttachmentCount = 0;
		SubpassDescription.pResolveAttachments = nullptr;

		VkRenderPassCreateInfo RenderPassCreateInfo;
		RenderPassCreateInfo.attachmentCount = 3;
		RenderPassCreateInfo.dependencyCount = 0;
		RenderPassCreateInfo.flags = 0;
		RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
		RenderPassCreateInfo.pDependencies = nullptr;
		RenderPassCreateInfo.pNext = nullptr;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.subpassCount = 1;

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &GBufferClearRenderPass));

		AttachmentDescriptions[0].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[1].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[2].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[2].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		
		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &GBufferDrawRenderPass));

		VkImageView FrameBufferAttachments[3] =
		{
			GBufferTexturesViews[0],
			GBufferTexturesViews[1],
			DepthBufferTextureView
		};

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 3;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = FrameBufferAttachments;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = GBufferDrawRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &GBufferFrameBuffer));

		VkBufferCreateInfo BufferCreateInfo;
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = 256 * 20000;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUConstantBuffer));

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUConstantBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPUConstantBuffer, GPUConstantBufferMemoryHeap, 0));

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers[0]));

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUConstantBufferMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers[0], CPUConstantBufferMemoryHeaps[0], 0));

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers[1]));

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUConstantBufferMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers[1], CPUConstantBufferMemoryHeaps[1], 0));
	}

	// ===============================================================================================================	

	{
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
		ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &ResolvedDepthBufferTexture));

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &ResolvedDepthBufferTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, ResolvedDepthBufferTexture, ResolvedDepthBufferTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		ImageViewCreateInfo.image = ResolvedDepthBufferTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &ResolvedDepthBufferTextureView));

		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		ImageViewCreateInfo.image = ResolvedDepthBufferTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &ResolvedDepthBufferTextureDepthOnlyView));

		VkAttachmentDescription2 AttachmentDescriptions[2];
		AttachmentDescriptions[0].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].flags = 0;
		AttachmentDescriptions[0].format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		AttachmentDescriptions[0].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[0].pNext = nullptr;
		AttachmentDescriptions[0].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[0].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[0].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[0].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[0].sType = VkStructureType::VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
		AttachmentDescriptions[1].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].flags = 0;
		AttachmentDescriptions[1].format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		AttachmentDescriptions[1].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[1].pNext = nullptr;
		AttachmentDescriptions[1].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescriptions[1].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[1].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[1].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[1].sType = VkStructureType::VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;

		VkAttachmentReference2 AttachmentReferences[2];
		AttachmentReferences[0].aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		AttachmentReferences[0].attachment = 0;
		AttachmentReferences[0].layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentReferences[0].pNext = nullptr;
		AttachmentReferences[0].sType = VkStructureType::VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
		AttachmentReferences[1].aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		AttachmentReferences[1].attachment = 1;
		AttachmentReferences[1].layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentReferences[1].pNext = nullptr;
		AttachmentReferences[1].sType = VkStructureType::VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;

		VkSubpassDescriptionDepthStencilResolve SubpassDescriptionDepthStencilResolve;
		SubpassDescriptionDepthStencilResolve.depthResolveMode = VkResolveModeFlagBits::VK_RESOLVE_MODE_MAX_BIT;
		SubpassDescriptionDepthStencilResolve.pDepthStencilResolveAttachment = &AttachmentReferences[1];
		SubpassDescriptionDepthStencilResolve.pNext = nullptr;
		SubpassDescriptionDepthStencilResolve.stencilResolveMode = VkResolveModeFlagBits::VK_RESOLVE_MODE_NONE;
		SubpassDescriptionDepthStencilResolve.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;

		VkSubpassDescription2 SubpassDescription;
		SubpassDescription.colorAttachmentCount = 0;
		SubpassDescription.flags = 0;
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = nullptr;
		SubpassDescription.pDepthStencilAttachment = &AttachmentReferences[0];
		SubpassDescription.pInputAttachments = nullptr;
		SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.pNext = &SubpassDescriptionDepthStencilResolve;
		SubpassDescription.pPreserveAttachments = nullptr;
		SubpassDescription.preserveAttachmentCount = 0;
		SubpassDescription.pResolveAttachments = nullptr;
		SubpassDescription.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
		SubpassDescription.viewMask = 0;

		VkRenderPassCreateInfo2 RenderPassCreateInfo;
		RenderPassCreateInfo.attachmentCount = 2;
		RenderPassCreateInfo.correlatedViewMaskCount = 0;
		RenderPassCreateInfo.dependencyCount = 0;
		RenderPassCreateInfo.flags = 0;
		RenderPassCreateInfo.pAttachments = AttachmentDescriptions;
		RenderPassCreateInfo.pCorrelatedViewMasks = nullptr;
		RenderPassCreateInfo.pDependencies = nullptr;
		RenderPassCreateInfo.pNext = nullptr;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		RenderPassCreateInfo.subpassCount = 1;

		PFN_vkCreateRenderPass2 vkCreateRenderPass2 = (PFN_vkCreateRenderPass2)vkGetDeviceProcAddr(Device, "vkCreateRenderPass2KHR");

		SAFE_VK(vkCreateRenderPass2(Device, &RenderPassCreateInfo, nullptr, &MSAADepthBufferResolveRenderPass));

		VkImageView FrameBufferAttachments[2] =
		{
			DepthBufferTextureView,
			ResolvedDepthBufferTextureView
		};

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 2;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = FrameBufferAttachments;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = MSAADepthBufferResolveRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &ResolvedDepthFrameBuffer));
	}

	// ===============================================================================================================	

	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.extent.height = 144;
		ImageCreateInfo.extent.width = 256;
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
		ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &OcclusionBufferTexture));

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, OcclusionBufferTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &OcclusionBufferTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, OcclusionBufferTexture, OcclusionBufferTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
		ImageViewCreateInfo.image = OcclusionBufferTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &OcclusionBufferTextureView));

		VkBufferCreateInfo BufferCreateInfo;
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = 256 * 144 * 4;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &OcclusionBufferReadbackBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &OcclusionBufferReadbackBuffers[1]));

		vkGetBufferMemoryRequirements(Device, OcclusionBufferReadbackBuffers[0], &MemoryRequirements);

		MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

			for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
			{
				if (((1 << i) & MemoryRequirements.memoryTypeBits) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
					return i;
			}

			return -1;

		} ();
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &OcclusionBufferReadbackBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, OcclusionBufferReadbackBuffers[0], OcclusionBufferReadbackBuffersMemoryHeaps[0], 0));

		vkGetBufferMemoryRequirements(Device, OcclusionBufferReadbackBuffers[0], &MemoryRequirements);

		MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocateInfo.memoryTypeIndex = [&] () -> uint32_t {

			for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
			{
				if (((1 << i) & MemoryRequirements.memoryTypeBits) && !(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
					return i;
			}

			return -1;

		} ();
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &OcclusionBufferReadbackBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, OcclusionBufferReadbackBuffers[1], OcclusionBufferReadbackBuffersMemoryHeaps[1], 0));

		VkAttachmentDescription AttachmentDescription;
		AttachmentDescription.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.flags = 0;
		AttachmentDescription.format = VkFormat::VK_FORMAT_R32_SFLOAT;
		AttachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescription.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescription.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference AttachmentReference;
		AttachmentReference.attachment = 0;
		AttachmentReference.layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkSubpassDescription SubpassDescription;
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.flags = 0;
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = &AttachmentReference;
		SubpassDescription.pDepthStencilAttachment = nullptr;
		SubpassDescription.pInputAttachments = nullptr;
		SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.pPreserveAttachments = nullptr;
		SubpassDescription.preserveAttachmentCount = 0;
		SubpassDescription.pResolveAttachments = nullptr;

		VkRenderPassCreateInfo RenderPassCreateInfo;
		RenderPassCreateInfo.attachmentCount = 1;
		RenderPassCreateInfo.dependencyCount = 0;
		RenderPassCreateInfo.flags = 0;
		RenderPassCreateInfo.pAttachments = &AttachmentDescription;
		RenderPassCreateInfo.pDependencies = nullptr;
		RenderPassCreateInfo.pNext = nullptr;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.subpassCount = 1;

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &OcclusionBufferRenderPass));

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = 144;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = &OcclusionBufferTextureView;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = OcclusionBufferRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = 256;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &OcclusionBufferFrameBuffer));

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[2];
		DescriptorSetLayoutBindings[0].binding = 0;
		DescriptorSetLayoutBindings[0].descriptorCount = 1;
		DescriptorSetLayoutBindings[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[0].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[1].binding = 1;
		DescriptorSetLayoutBindings[1].descriptorCount = 1;
		DescriptorSetLayoutBindings[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		DescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[1].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
		DescriptorSetLayoutCreateInfo.bindingCount = 2;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &OcclusionBufferSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &OcclusionBufferSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &OcclusionBufferPipelineLayout));

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &OcclusionBufferSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &OcclusionBufferSets[0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &OcclusionBufferSets[1]));

		VkShaderModule OcclusionBufferShaderModule;

		HANDLE OcclusionBufferPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/OcclusionBuffer.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER OcclusionBufferPixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(OcclusionBufferPixelShaderFile, &OcclusionBufferPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> OcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(OcclusionBufferPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(OcclusionBufferPixelShaderFile, OcclusionBufferPixelShaderByteCodeData, (DWORD)OcclusionBufferPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(OcclusionBufferPixelShaderFile);

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = OcclusionBufferPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = OcclusionBufferPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &OcclusionBufferShaderModule));

		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState;
		ZeroMemory(&PipelineColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
		PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
		PipelineColorBlendAttachmentState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo;
		ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
		PipelineColorBlendStateCreateInfo.attachmentCount = 1;
		PipelineColorBlendStateCreateInfo.flags = 0;
		PipelineColorBlendStateCreateInfo.logicOp = (VkLogicOp)0;
		PipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
		PipelineColorBlendStateCreateInfo.pNext = nullptr;
		PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo;
		ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
		PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
		PipelineDepthStencilStateCreateInfo.pNext = nullptr;
		PipelineDepthStencilStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkDynamicState DynamicStates[4] =
		{
			VkDynamicState::VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			VkDynamicState::VK_DYNAMIC_STATE_STENCIL_REFERENCE,
			VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
			VkDynamicState::VK_DYNAMIC_STATE_SCISSOR
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
		PipelineInputAssemblyStateCreateInfo.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo;
		ZeroMemory(&PipelineMultisampleStateCreateInfo, sizeof(VkPipelineMultisampleStateCreateInfo));
		PipelineMultisampleStateCreateInfo.pNext = nullptr;
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		PipelineMultisampleStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo;
		ZeroMemory(&PipelineRasterizationStateCreateInfo, sizeof(PipelineRasterizationStateCreateInfo));
		PipelineRasterizationStateCreateInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
		PipelineRasterizationStateCreateInfo.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
		PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
		PipelineRasterizationStateCreateInfo.pNext = nullptr;
		PipelineRasterizationStateCreateInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
		PipelineRasterizationStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfos[2];
		PipelineShaderStageCreateInfos[0].flags = 0;
		PipelineShaderStageCreateInfos[0].module = FullScreenQuadShaderModule;
		PipelineShaderStageCreateInfos[0].pName = "VS";
		PipelineShaderStageCreateInfos[0].pNext = nullptr;
		PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		PipelineShaderStageCreateInfos[1].flags = 0;
		PipelineShaderStageCreateInfos[1].module = OcclusionBufferShaderModule;
		PipelineShaderStageCreateInfos[1].pName = "PS";
		PipelineShaderStageCreateInfos[1].pNext = nullptr;
		PipelineShaderStageCreateInfos[1].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		PipelineShaderStageCreateInfos[1].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;
		PipelineVertexInputStateCreateInfo.flags = 0;
		PipelineVertexInputStateCreateInfo.pNext = nullptr;
		PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
		PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;

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
		GraphicsPipelineCreateInfo.layout = OcclusionBufferPipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = OcclusionBufferRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &OcclusionBufferPipeline));
	}

	// ===============================================================================================================	

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

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[0]));
		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[1]));
		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[2]));
		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &CascadedShadowMapTextures[3]));
		
		VkMemoryRequirements MemoryRequirements;
		VkMemoryAllocateInfo MemoryAllocateInfo;

		for (int k = 0; k < 4; k++)
		{
			vkGetImageMemoryRequirements(Device, CascadedShadowMapTextures[k], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CascadedShadowMapTexturesMemoryHeaps[k]));

			SAFE_VK(vkBindImageMemory(Device, CascadedShadowMapTextures[k], CascadedShadowMapTexturesMemoryHeaps[k], 0));
		}

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_D32_SFLOAT;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		ImageViewCreateInfo.image = CascadedShadowMapTextures[0];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &CascadedShadowMapTexturesViews[0]));
		ImageViewCreateInfo.image = CascadedShadowMapTextures[1];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &CascadedShadowMapTexturesViews[1]));
		ImageViewCreateInfo.image = CascadedShadowMapTextures[2];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &CascadedShadowMapTexturesViews[2]));
		ImageViewCreateInfo.image = CascadedShadowMapTextures[3];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &CascadedShadowMapTexturesViews[3]));
		
		VkAttachmentDescription AttachmentDescription;
		AttachmentDescription.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescription.flags = 0;
		AttachmentDescription.format = VkFormat::VK_FORMAT_D32_SFLOAT;
		AttachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
		AttachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescription.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescription.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference AttachmentReference;
		AttachmentReference.attachment = 0;
		AttachmentReference.layout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription SubpassDescription;
		SubpassDescription.colorAttachmentCount = 0;
		SubpassDescription.flags = 0;
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = nullptr;
		SubpassDescription.pDepthStencilAttachment = &AttachmentReference;
		SubpassDescription.pInputAttachments = nullptr;
		SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.pPreserveAttachments = nullptr;
		SubpassDescription.preserveAttachmentCount = 0;
		SubpassDescription.pResolveAttachments = nullptr;

		VkRenderPassCreateInfo RenderPassCreateInfo;
		RenderPassCreateInfo.attachmentCount = 1;
		RenderPassCreateInfo.dependencyCount = 0;
		RenderPassCreateInfo.flags = 0;
		RenderPassCreateInfo.pAttachments = &AttachmentDescription;
		RenderPassCreateInfo.pDependencies = nullptr;
		RenderPassCreateInfo.pNext = nullptr;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.subpassCount = 1;

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &ShadowMapClearRenderPass));

		AttachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &ShadowMapDrawRenderPass));

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = 2048;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = ShadowMapDrawRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = 2048;

		FramebufferCreateInfo.pAttachments = &CascadedShadowMapTexturesViews[0];
		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &CascadedShadowMapFrameBuffers[0]));
		FramebufferCreateInfo.pAttachments = &CascadedShadowMapTexturesViews[1];
		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &CascadedShadowMapFrameBuffers[1]));
		FramebufferCreateInfo.pAttachments = &CascadedShadowMapTexturesViews[2];
		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &CascadedShadowMapFrameBuffers[2]));
		FramebufferCreateInfo.pAttachments = &CascadedShadowMapTexturesViews[3];
		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &CascadedShadowMapFrameBuffers[3]));

		VkBufferCreateInfo BufferCreateInfo;
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = 256 * 20000;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUConstantBuffers2[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUConstantBuffers2[1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUConstantBuffers2[2]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUConstantBuffers2[3]));

		for (int k = 0; k < 4; k++)
		{
			vkGetBufferMemoryRequirements(Device, GPUConstantBuffers2[k], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUConstantBufferMemoryHeaps2[k]));

			SAFE_VK(vkBindBufferMemory(Device, GPUConstantBuffers2[k], GPUConstantBufferMemoryHeaps2[k], 0));
		}

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[0][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[0][1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[1][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[1][1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[2][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[2][1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[3][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[3][1]));

		for (int k = 0; k < 4; k++)
		{
			vkGetBufferMemoryRequirements(Device, GPUConstantBuffers2[k], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUConstantBufferMemoryHeaps2[k][0]));

			SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[k][0], CPUConstantBufferMemoryHeaps2[k][0], 0));

			SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[k][1]));

			vkGetBufferMemoryRequirements(Device, GPUConstantBuffers2[k], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUConstantBufferMemoryHeaps2[k][1]));

			SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[k][1], CPUConstantBufferMemoryHeaps2[k][1], 0));
		}
	}

	// ===============================================================================================================	

	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.extent.height = ResolutionHeight;
		ImageCreateInfo.extent.width = ResolutionWidth;
		ImageCreateInfo.flags = 0;
		ImageCreateInfo.format = VkFormat::VK_FORMAT_R8_UNORM;
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
		ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &ShadowMaskTexture));

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, ShadowMaskTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &ShadowMaskTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, ShadowMaskTexture, ShadowMaskTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R8_UNORM;
		ImageViewCreateInfo.image = ShadowMaskTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &ShadowMaskTextureView));

		VkAttachmentDescription AttachmentDescription;
		AttachmentDescription.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.flags = 0;
		AttachmentDescription.format = VkFormat::VK_FORMAT_R8_UNORM;
		AttachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescription.stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescription.stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescription.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference AttachmentReference;
		AttachmentReference.attachment = 0;
		AttachmentReference.layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription SubpassDescription;
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.flags = 0;
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = &AttachmentReference;
		SubpassDescription.pDepthStencilAttachment = nullptr;
		SubpassDescription.pInputAttachments = nullptr;
		SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.pPreserveAttachments = nullptr;
		SubpassDescription.preserveAttachmentCount = 0;
		SubpassDescription.pResolveAttachments = nullptr;

		VkRenderPassCreateInfo RenderPassCreateInfo;
		RenderPassCreateInfo.attachmentCount = 1;
		RenderPassCreateInfo.dependencyCount = 0;
		RenderPassCreateInfo.flags = 0;
		RenderPassCreateInfo.pAttachments = &AttachmentDescription;
		RenderPassCreateInfo.pDependencies = nullptr;
		RenderPassCreateInfo.pNext = nullptr;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		RenderPassCreateInfo.subpassCount = 1;

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &ShadowMaskRenderPass));

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = &ShadowMaskTextureView;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = ShadowMaskRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &ShadowMaskFrameBuffer));

		VkBufferCreateInfo BufferCreateInfo;
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = 256;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUShadowResolveConstantBuffer));

		vkGetBufferMemoryRequirements(Device, GPUShadowResolveConstantBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUShadowResolveConstantBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPUShadowResolveConstantBuffer, GPUShadowResolveConstantBufferMemoryHeap, 0));

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUShadowResolveConstantBuffers[0]));

		vkGetBufferMemoryRequirements(Device, CPUShadowResolveConstantBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUShadowResolveConstantBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPUShadowResolveConstantBuffers[0], CPUShadowResolveConstantBuffersMemoryHeaps[0], 0));

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUShadowResolveConstantBuffers[1]));

		vkGetBufferMemoryRequirements(Device, CPUShadowResolveConstantBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUShadowResolveConstantBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPUShadowResolveConstantBuffers[1], CPUShadowResolveConstantBuffersMemoryHeaps[1], 0));

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[4];
		DescriptorSetLayoutBindings[0].binding = 0;
		DescriptorSetLayoutBindings[0].descriptorCount = 1;
		DescriptorSetLayoutBindings[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		DescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[0].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[1].binding = 1;
		DescriptorSetLayoutBindings[1].descriptorCount = 1;
		DescriptorSetLayoutBindings[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[1].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[2].binding = 2;
		DescriptorSetLayoutBindings[2].descriptorCount = 4;
		DescriptorSetLayoutBindings[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[2].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[3].binding = 3;
		DescriptorSetLayoutBindings[3].descriptorCount = 1;
		DescriptorSetLayoutBindings[3].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		DescriptorSetLayoutBindings[3].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[3].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
		DescriptorSetLayoutCreateInfo.bindingCount = 4;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &ShadowResolveSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &ShadowResolveSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &ShadowResolvePipelineLayout));

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &ShadowResolveSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ShadowResolveSets[0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &ShadowResolveSets[1]));

		VkShaderModule ShadowResolveShaderModule;

		HANDLE ShadowResolvePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ShadowResolve.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ShadowResolvePixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(ShadowResolvePixelShaderFile, &ShadowResolvePixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> ShadowResolvePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowResolvePixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(ShadowResolvePixelShaderFile, ShadowResolvePixelShaderByteCodeData, (DWORD)ShadowResolvePixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(ShadowResolvePixelShaderFile);

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = ShadowResolvePixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = ShadowResolvePixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ShadowResolveShaderModule));

		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState;
		ZeroMemory(&PipelineColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
		PipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
		PipelineColorBlendAttachmentState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo;
		ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
		PipelineColorBlendStateCreateInfo.attachmentCount = 1;
		PipelineColorBlendStateCreateInfo.flags = 0;
		PipelineColorBlendStateCreateInfo.logicOp = (VkLogicOp)0;
		PipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
		PipelineColorBlendStateCreateInfo.pNext = nullptr;
		PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo;
		ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
		PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
		PipelineDepthStencilStateCreateInfo.pNext = nullptr;
		PipelineDepthStencilStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		VkDynamicState DynamicStates[4] =
		{
			VkDynamicState::VK_DYNAMIC_STATE_BLEND_CONSTANTS,
			VkDynamicState::VK_DYNAMIC_STATE_STENCIL_REFERENCE,
			VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
			VkDynamicState::VK_DYNAMIC_STATE_SCISSOR
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
		PipelineInputAssemblyStateCreateInfo.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo;
		ZeroMemory(&PipelineMultisampleStateCreateInfo, sizeof(VkPipelineMultisampleStateCreateInfo));
		PipelineMultisampleStateCreateInfo.pNext = nullptr;
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		PipelineMultisampleStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo;
		ZeroMemory(&PipelineRasterizationStateCreateInfo, sizeof(PipelineRasterizationStateCreateInfo));
		PipelineRasterizationStateCreateInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
		PipelineRasterizationStateCreateInfo.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
		PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
		PipelineRasterizationStateCreateInfo.pNext = nullptr;
		PipelineRasterizationStateCreateInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
		PipelineRasterizationStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfos[2];
		PipelineShaderStageCreateInfos[0].flags = 0;
		PipelineShaderStageCreateInfos[0].module = FullScreenQuadShaderModule;
		PipelineShaderStageCreateInfos[0].pName = "VS";
		PipelineShaderStageCreateInfos[0].pNext = nullptr;
		PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		PipelineShaderStageCreateInfos[1].flags = 0;
		PipelineShaderStageCreateInfos[1].module = ShadowResolveShaderModule;
		PipelineShaderStageCreateInfos[1].pName = "PS";
		PipelineShaderStageCreateInfos[1].pNext = nullptr;
		PipelineShaderStageCreateInfos[1].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		PipelineShaderStageCreateInfos[1].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;
		PipelineVertexInputStateCreateInfo.flags = 0;
		PipelineVertexInputStateCreateInfo.pNext = nullptr;
		PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
		PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;

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
		GraphicsPipelineCreateInfo.layout = ShadowResolvePipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = ShadowMaskRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &ShadowResolvePipeline));
	}
}

void RenderSystem::ShutdownSystem()
{
	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;

	SAFE_VK(vkWaitForFences(Device, 1, &FrameSyncFences[CurrentFrameIndex], VK_FALSE, UINT64_MAX));

	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	{
		vkDestroyBuffer(Device, renderMesh->VertexBuffer, nullptr);
		vkDestroyBuffer(Device, renderMesh->IndexBuffer, nullptr);

		delete renderMesh;
	}

	RenderMeshDestructionQueue.clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	{
		vkDestroyPipeline(Device, renderMaterial->GBufferOpaquePassPipeline, nullptr);
		vkDestroyPipeline(Device, renderMaterial->ShadowMapPassPipeline, nullptr);

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

	for (uint32_t i = 0; i < SwapChainImagesCount; i++)
	{
		vkDestroyImageView(Device, BackBufferTexturesViews[i], nullptr);
	}
	
	vkDestroyImageView(Device, DepthBufferTextureView, nullptr);
	vkDestroyImage(Device, DepthBufferTexture, nullptr);
	vkFreeMemory(Device, DepthBufferTextureMemoryHeap, nullptr);

	vkDestroyFence(Device, FrameSyncFences[0], nullptr);
	vkDestroyFence(Device, FrameSyncFences[1], nullptr);
	vkDestroyFence(Device, CopySyncFence, nullptr);
	vkDestroySemaphore(Device, ImageAvailabilitySemaphore, nullptr);
	vkDestroySemaphore(Device, ImagePresentationSemaphore, nullptr);

	vkDestroySampler(Device, TextureSampler, nullptr);

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

	SAFE_VK(vkResetDescriptorPool(Device, DescriptorPools[0], 0));
	SAFE_VK(vkResetDescriptorPool(Device, DescriptorPools[1], 0));

	vkDestroyDescriptorPool(Device, DescriptorPools[0], nullptr);
	vkDestroyDescriptorPool(Device, DescriptorPools[1], nullptr);

	vkFreeCommandBuffers(Device, CommandPool, 2, CommandBuffers);

	vkDestroyCommandPool(Device, CommandPool, nullptr);

	vkDestroySwapchainKHR(Device, SwapChain, nullptr);
	vkDestroySurfaceKHR(Instance, Surface, nullptr);

	for (uint32_t i = 0; i < SwapChainImagesCount; i++)
	{
		//vkDestroyFramebuffer(Device, FrameBuffers[i], nullptr);
	}

	//vkDestroyRenderPass(Device, RenderPass, nullptr);

#ifdef _DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");

	vkDestroyDebugUtilsMessengerEXT(Instance, DebugUtilsMessenger, nullptr);
#endif

	vkDestroyDevice(Device, nullptr);
	vkDestroyInstance(Instance, nullptr);
}

void RenderSystem::TickSystem(float DeltaTime)
{
	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	Camera& camera = gameFramework.GetCamera();

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();

	XMMATRIX ShadowViewMatrices[4], ShadowProjMatrices[4], ShadowViewProjMatrices[4];

	ShadowViewMatrices[0] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 10.0f, CameraLocation.y + 10.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[1] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 20.0f, CameraLocation.y + 20.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[2] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 50.0f, CameraLocation.y + 50.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[3] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 100.0f, CameraLocation.y + 100.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ShadowProjMatrices[0] = XMMatrixOrthographicLH(10.0f, 10.0f, 0.01f, 500.0f);
	ShadowProjMatrices[1] = XMMatrixOrthographicLH(20.0f, 20.0f, 0.01f, 500.0f);
	ShadowProjMatrices[2] = XMMatrixOrthographicLH(50.0f, 50.0f, 0.01f, 500.0f);
	ShadowProjMatrices[3] = XMMatrixOrthographicLH(100.0f, 100.0f, 0.01f, 500.0f);

	ShadowViewProjMatrices[0] = ShadowViewMatrices[0] * ShadowProjMatrices[0];
	ShadowViewProjMatrices[1] = ShadowViewMatrices[1] * ShadowProjMatrices[1];
	ShadowViewProjMatrices[2] = ShadowViewMatrices[2] * ShadowProjMatrices[2];
	ShadowViewProjMatrices[3] = ShadowViewMatrices[3] * ShadowProjMatrices[3];

	RenderScene& renderScene = gameFramework.GetWorld().GetRenderScene();

	vector<StaticMeshComponent*> AllStaticMeshComponents = renderScene.GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	vector<PointLightComponent*> AllPointLightComponents = renderScene.GetPointLightComponents();
	vector<PointLightComponent*> VisblePointLightComponents = cullingSubSystem.GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

	//clusterizationSubSystem.ClusterizeLights(VisblePointLightComponents, ViewMatrix);

	/*vector<PointLight> PointLights;

	for (PointLightComponent *pointLightComponent : VisblePointLightComponents)
	{
		PointLight pointLight;
		pointLight.Brightness = pointLightComponent->GetBrightness();
		pointLight.Color = pointLightComponent->GetColor();
		pointLight.Position = pointLightComponent->GetTransformComponent()->GetLocation();
		pointLight.Radius = pointLightComponent->GetRadius();

		PointLights.push_back(pointLight);
	}*/

	SAFE_VK(vkWaitForFences(Device, 1, &FrameSyncFences[CurrentFrameIndex], VK_FALSE, UINT64_MAX));

	SAFE_VK(vkResetFences(Device, 1, &FrameSyncFences[CurrentFrameIndex]));

	SAFE_VK(vkAcquireNextImageKHR(Device, SwapChain, UINT64_MAX, ImageAvailabilitySemaphore, VK_NULL_HANDLE, &CurrentBackBufferIndex));

	VkCommandBufferBeginInfo CommandBufferBeginInfo;
	CommandBufferBeginInfo.flags = VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;
	CommandBufferBeginInfo.pNext = nullptr;
	CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	SAFE_VK(vkBeginCommandBuffer(CommandBuffers[CurrentFrameIndex], &CommandBufferBeginInfo));

	{
		void *ConstantBufferData;
		size_t ConstantBufferOffset = 0;

		SAFE_VK(vkMapMemory(Device, CPUConstantBufferMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &ConstantBufferData));

		for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
			XMMATRIX WVPMatrix = WorldMatrix * ViewProjMatrix;

			XMFLOAT3X4 VectorTransformMatrix;

			float Determinant =
				WorldMatrix.m[0][0] * (WorldMatrix.m[1][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[1][2]) -
				WorldMatrix.m[1][0] * (WorldMatrix.m[0][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[0][2]) +
				WorldMatrix.m[2][0] * (WorldMatrix.m[0][1] * WorldMatrix.m[1][2] - WorldMatrix.m[1][1] * WorldMatrix.m[0][2]);

			VectorTransformMatrix.m[0][0] = (WorldMatrix.m[1][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[1][2]) / Determinant;
			VectorTransformMatrix.m[1][0] = -(WorldMatrix.m[0][1] * WorldMatrix.m[2][2] - WorldMatrix.m[2][1] * WorldMatrix.m[0][2]) / Determinant;
			VectorTransformMatrix.m[2][0] = (WorldMatrix.m[0][1] * WorldMatrix.m[1][2] - WorldMatrix.m[1][1] * WorldMatrix.m[0][2]) / Determinant;

			VectorTransformMatrix.m[0][1] = -(WorldMatrix.m[1][0] * WorldMatrix.m[2][2] - WorldMatrix.m[2][0] * WorldMatrix.m[1][2]) / Determinant;
			VectorTransformMatrix.m[1][1] = (WorldMatrix.m[0][0] * WorldMatrix.m[2][2] - WorldMatrix.m[2][0] * WorldMatrix.m[0][2]) / Determinant;
			VectorTransformMatrix.m[2][1] = -(WorldMatrix.m[0][0] * WorldMatrix.m[1][0] - WorldMatrix.m[0][2] * WorldMatrix.m[1][2]) / Determinant;

			VectorTransformMatrix.m[0][2] = (WorldMatrix.m[1][0] * WorldMatrix.m[2][1] - WorldMatrix.m[2][0] * WorldMatrix.m[1][1]) / Determinant;
			VectorTransformMatrix.m[1][2] = -(WorldMatrix.m[0][0] * WorldMatrix.m[2][1] - WorldMatrix.m[2][0] * WorldMatrix.m[0][1]) / Determinant;
			VectorTransformMatrix.m[2][2] = (WorldMatrix.m[0][0] * WorldMatrix.m[1][1] - WorldMatrix.m[1][0] * WorldMatrix.m[0][1]) / Determinant;

			VectorTransformMatrix.m[0][3] = 0.0f;
			VectorTransformMatrix.m[1][3] = 0.0f;
			VectorTransformMatrix.m[2][3] = 0.0f;

			memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset, &WVPMatrix, sizeof(XMMATRIX));
			memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset + sizeof(XMMATRIX), &WorldMatrix, sizeof(XMMATRIX));
			memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset + 2 * sizeof(XMMATRIX), &VectorTransformMatrix, sizeof(XMFLOAT3X4));

			ConstantBufferOffset += 256;
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

		VkImageMemoryBarrier ImageMemoryBarriers[3];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = GBufferTextures[0];
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
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = GBufferTextures[1];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[1].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[1].pNext = nullptr;
		ImageMemoryBarriers[1].srcAccessMask = 0;
		ImageMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[1].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[1].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[1].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[1].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[1].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[2].dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].image = DepthBufferTexture;
		ImageMemoryBarriers[2].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[2].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[2].pNext = nullptr;
		ImageMemoryBarriers[2].srcAccessMask = 0;
		ImageMemoryBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[2].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		ImageMemoryBarriers[2].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[2].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[2].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[2].subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 3, ImageMemoryBarriers);

		VkClearValue ClearValues[3];
		ClearValues[0].color.float32[0] = 0.0f;
		ClearValues[0].color.float32[1] = 0.0f;
		ClearValues[0].color.float32[2] = 0.0f;
		ClearValues[0].color.float32[3] = 0.0f;
		ClearValues[1].color.float32[0] = 0.0f;
		ClearValues[1].color.float32[1] = 0.0f;
		ClearValues[1].color.float32[2] = 0.0f;
		ClearValues[1].color.float32[3] = 0.0f;
		ClearValues[2].depthStencil.depth = 0.0f;
		ClearValues[2].depthStencil.stencil = 0;

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 3;
		RenderPassBeginInfo.framebuffer = GBufferFrameBuffer;
		RenderPassBeginInfo.pClearValues = ClearValues;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = GBufferClearRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = GBufferFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = GBufferDrawRenderPass;
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
		DescriptorImageInfo.sampler = TextureSampler;

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
			RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
			RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

			VkDescriptorBufferInfo DescriptorBufferInfo;
			DescriptorBufferInfo.buffer = GPUConstantBuffer;
			DescriptorBufferInfo.offset = 256 * k;
			DescriptorBufferInfo.range = 256;

			VkDescriptorImageInfo DescriptorImageInfos[2];
			DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DescriptorImageInfos[0].imageView = renderTexture0->TextureView;
			DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
			DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DescriptorImageInfos[1].imageView = renderTexture1->TextureView;
			DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

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
			WriteDescriptorSets[1].descriptorCount = 2;
			WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			WriteDescriptorSets[1].dstArrayElement = 0;
			WriteDescriptorSets[1].dstBinding = 0;
			WriteDescriptorSets[1].dstSet = TexturesSets[CurrentFrameIndex][k];
			WriteDescriptorSets[1].pBufferInfo = nullptr;
			WriteDescriptorSets[1].pImageInfo = DescriptorImageInfos;
			WriteDescriptorSets[1].pNext = nullptr;
			WriteDescriptorSets[1].pTexelBufferView = nullptr;
			WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

			vkCmdBindVertexBuffers(CommandBuffers[CurrentFrameIndex], 0, 1, &renderMesh->VertexBuffer, &Offset);
			vkCmdBindIndexBuffer(CommandBuffers[CurrentFrameIndex], renderMesh->IndexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);

			vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, renderMaterial->GBufferOpaquePassPipeline);

			VkDescriptorSet DescriptorSets[2] =
			{
				ConstantBuffersSets[CurrentFrameIndex][k],
				TexturesSets[CurrentFrameIndex][k]
			};

			vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 2, DescriptorSets, 0, nullptr);

			vkCmdDrawIndexed(CommandBuffers[CurrentFrameIndex], 8 * 8 * 6 * 6, 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
	}

	// ===============================================================================================================	

	{
		VkImageMemoryBarrier ImageMemoryBarriers[1];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = ResolvedDepthBufferTexture;
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = 0;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, ImageMemoryBarriers);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = ResolvedDepthFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = MSAADepthBufferResolveRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
	}

	// ===============================================================================================================	



	// ===============================================================================================================	

	{
		VkImageMemoryBarrier ImageMemoryBarriers[2];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = ResolvedDepthBufferTexture;
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;

		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = OcclusionBufferTexture;
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[1].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[1].pNext = nullptr;
		ImageMemoryBarriers[1].srcAccessMask = 0;
		ImageMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[1].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[1].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[1].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[1].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[1].subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = OcclusionBufferFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = 144;
		RenderPassBeginInfo.renderArea.extent.width = 256;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = OcclusionBufferRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

		VkViewport Viewport;
		Viewport.height = 144.0f;
		Viewport.maxDepth = 1.0f;
		Viewport.minDepth = 0.0f;
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;
		Viewport.width = 256.0f;

		vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

		VkRect2D ScissorRect;
		ScissorRect.extent.height = 144;
		ScissorRect.offset.x = 0;
		ScissorRect.extent.width = 256;
		ScissorRect.offset.y = 0;

		vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

		VkDescriptorImageInfo DescriptorImageInfos[2];
		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = ResolvedDepthBufferTextureDepthOnlyView;
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = (VkImageLayout)0;
		DescriptorImageInfos[1].imageView = VK_NULL_HANDLE;
		DescriptorImageInfos[1].sampler = MinSampler;

		VkWriteDescriptorSet WriteDescriptorSets[2];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = OcclusionBufferSets[CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = OcclusionBufferSets[CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, OcclusionBufferPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, OcclusionBufferPipelineLayout, 0, 1, &OcclusionBufferSets[CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = OcclusionBufferTexture;
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
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

		VkBufferMemoryBarrier BufferMemoryBarrier;
		BufferMemoryBarrier.buffer = OcclusionBufferReadbackBuffers[CurrentFrameIndex];
		BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.offset = 0;
		BufferMemoryBarrier.pNext = nullptr;
		BufferMemoryBarrier.size = VK_WHOLE_SIZE;
		BufferMemoryBarrier.srcAccessMask = 0;
		BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 1, ImageMemoryBarriers);

		VkBufferImageCopy BufferImageCopy;
		BufferImageCopy.bufferImageHeight = 144;
		BufferImageCopy.bufferOffset = 0;
		BufferImageCopy.bufferRowLength = 256;
		BufferImageCopy.imageExtent.depth = 1;
		BufferImageCopy.imageExtent.height = 144;
		BufferImageCopy.imageExtent.width = 256;
		BufferImageCopy.imageOffset.x = 0;
		BufferImageCopy.imageOffset.y = 0;
		BufferImageCopy.imageOffset.z = 0;
		BufferImageCopy.imageSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		BufferImageCopy.imageSubresource.baseArrayLayer = 0;
		BufferImageCopy.imageSubresource.layerCount = 1;
		BufferImageCopy.imageSubresource.mipLevel = 0;

		vkCmdCopyImageToBuffer(CommandBuffers[CurrentFrameIndex], OcclusionBufferTexture, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, OcclusionBufferReadbackBuffers[CurrentFrameIndex], 1, &BufferImageCopy);

		BufferMemoryBarrier.buffer = OcclusionBufferReadbackBuffers[CurrentFrameIndex];
		BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_READ_BIT;
		BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.offset = 0;
		BufferMemoryBarrier.pNext = nullptr;
		BufferMemoryBarrier.size = VK_WHOLE_SIZE;
		BufferMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

		float *MappedData;

		SAFE_VK(vkMapMemory(Device, OcclusionBufferReadbackBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, (void**)&MappedData));

		for (int i = 0; i < 144; i++)
		{
			memcpy(cullingSubSystem.GetOcclusionBufferData() + i * 256, MappedData + i * 256, 256 * 4);
		}

		vkUnmapMemory(Device, OcclusionBufferReadbackBuffersMemoryHeaps[CurrentFrameIndex]);
	}

	// ===============================================================================================================	

	// ===============================================================================================================	

	{
		VkImageMemoryBarrier ImageMemoryBarriers[4];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = CascadedShadowMapTextures[0];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = 0;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = CascadedShadowMapTextures[1];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[1].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[1].pNext = nullptr;
		ImageMemoryBarriers[1].srcAccessMask = 0;
		ImageMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[1].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[1].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[1].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[1].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[1].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[2].dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].image = CascadedShadowMapTextures[2];
		ImageMemoryBarriers[2].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[2].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[2].pNext = nullptr;
		ImageMemoryBarriers[2].srcAccessMask = 0;
		ImageMemoryBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[2].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[2].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[2].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[2].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[2].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[3].dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[3].image = CascadedShadowMapTextures[3];
		ImageMemoryBarriers[3].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[3].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[3].pNext = nullptr;
		ImageMemoryBarriers[3].srcAccessMask = 0;
		ImageMemoryBarriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[3].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[3].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[3].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[3].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[3].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[3].subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 4, ImageMemoryBarriers);

		for (int i = 0; i < 4; i++)
		{
			vector<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
			vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i], false);
			size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

			void *ConstantBufferData;
			size_t ConstantBufferOffset = 0;

			SAFE_VK(vkMapMemory(Device, CPUConstantBufferMemoryHeaps2[i][CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &ConstantBufferData));

			for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
			{
				XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
				XMMATRIX WVPMatrix = WorldMatrix * ShadowViewProjMatrices[i];

				memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset, &WVPMatrix, sizeof(XMMATRIX));

				ConstantBufferOffset += 256;
			}

			vkUnmapMemory(Device, CPUConstantBufferMemoryHeaps2[i][CurrentFrameIndex]);

			VkBufferMemoryBarrier BufferMemoryBarrier;
			BufferMemoryBarrier.buffer = CPUConstantBuffers2[i][CurrentFrameIndex];
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

				vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPUConstantBuffers2[i][CurrentFrameIndex], GPUConstantBuffers2[i], 1, &BufferCopy);
			}

			BufferMemoryBarrier.buffer = GPUConstantBuffers2[i];
			BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT;
			BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			BufferMemoryBarrier.offset = 0;
			BufferMemoryBarrier.pNext = nullptr;
			BufferMemoryBarrier.size = VK_WHOLE_SIZE;
			BufferMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
			BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

			vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

			VkClearValue ClearValue;
			ClearValue.depthStencil.depth = 1.0f;
			ClearValue.depthStencil.stencil = 0;

			VkRenderPassBeginInfo RenderPassBeginInfo;
			RenderPassBeginInfo.clearValueCount = 1;
			RenderPassBeginInfo.framebuffer = CascadedShadowMapFrameBuffers[i];
			RenderPassBeginInfo.pClearValues = &ClearValue;
			RenderPassBeginInfo.pNext = nullptr;
			RenderPassBeginInfo.renderArea.extent.height = 2048;
			RenderPassBeginInfo.renderArea.extent.width = 2048;
			RenderPassBeginInfo.renderArea.offset.x = 0;
			RenderPassBeginInfo.renderArea.offset.y = 0;
			RenderPassBeginInfo.renderPass = ShadowMapClearRenderPass;
			RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

			vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
			vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

			RenderPassBeginInfo.clearValueCount = 0;
			RenderPassBeginInfo.framebuffer = CascadedShadowMapFrameBuffers[i];
			RenderPassBeginInfo.pClearValues = nullptr;
			RenderPassBeginInfo.pNext = nullptr;
			RenderPassBeginInfo.renderArea.extent.height = 2048;
			RenderPassBeginInfo.renderArea.extent.width = 2048;
			RenderPassBeginInfo.renderArea.offset.x = 0;
			RenderPassBeginInfo.renderArea.offset.y = 0;
			RenderPassBeginInfo.renderPass = ShadowMapDrawRenderPass;
			RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

			vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

			VkViewport Viewport;
			Viewport.height = 2048.0f;
			Viewport.maxDepth = 1.0f;
			Viewport.minDepth = 0.0f;
			Viewport.x = 0.0f;
			Viewport.y = 0.0f;
			Viewport.width = 2048.0f;

			vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

			VkRect2D ScissorRect;
			ScissorRect.extent.height = 2048;
			ScissorRect.offset.x = 0;
			ScissorRect.extent.width = 2048;
			ScissorRect.offset.y = 0;

			vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

			VkDeviceSize Offset = 0;

			for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
			{
				StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

				RenderMesh *renderMesh = staticMeshComponent->GetStaticMesh()->GetRenderMesh();
				RenderMaterial *renderMaterial = staticMeshComponent->GetMaterial()->GetRenderMaterial();
				RenderTexture *renderTexture0 = staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
				RenderTexture *renderTexture1 = staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

				VkDescriptorBufferInfo DescriptorBufferInfo;
				DescriptorBufferInfo.buffer = GPUConstantBuffers2[i];
				DescriptorBufferInfo.offset = 256 * k;
				DescriptorBufferInfo.range = 256;

				VkWriteDescriptorSet WriteDescriptorSet;
				WriteDescriptorSet.descriptorCount = 1;
				WriteDescriptorSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				WriteDescriptorSet.dstArrayElement = 0;
				WriteDescriptorSet.dstBinding = 0;
				WriteDescriptorSet.dstSet = ConstantBuffersSets[CurrentFrameIndex][20000 * (i + 1) + k];
				WriteDescriptorSet.pBufferInfo = &DescriptorBufferInfo;
				WriteDescriptorSet.pImageInfo = nullptr;
				WriteDescriptorSet.pNext = nullptr;
				WriteDescriptorSet.pTexelBufferView = nullptr;
				WriteDescriptorSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

				vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);

				vkCmdBindVertexBuffers(CommandBuffers[CurrentFrameIndex], 0, 1, &renderMesh->VertexBuffer, &Offset);
				vkCmdBindIndexBuffer(CommandBuffers[CurrentFrameIndex], renderMesh->IndexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);

				vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, renderMaterial->ShadowMapPassPipeline);

				vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &ConstantBuffersSets[CurrentFrameIndex][20000 * (i + 1) + k], 0, nullptr);

				vkCmdDrawIndexed(CommandBuffers[CurrentFrameIndex], 8 * 8 * 6 * 6, 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
		}
	}

	// ===============================================================================================================	

	// ===============================================================================================================	

	{
		VkImageMemoryBarrier ImageMemoryBarriers[5];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = CascadedShadowMapTextures[0];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = CascadedShadowMapTextures[1];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[1].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[1].pNext = nullptr;
		ImageMemoryBarriers[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[1].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[1].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[1].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[1].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[1].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[2].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].image = CascadedShadowMapTextures[2];
		ImageMemoryBarriers[2].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[2].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[2].pNext = nullptr;
		ImageMemoryBarriers[2].srcAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[2].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[2].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[2].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[2].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[2].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[3].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[3].image = CascadedShadowMapTextures[3];
		ImageMemoryBarriers[3].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[3].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[3].pNext = nullptr;
		ImageMemoryBarriers[3].srcAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[3].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[3].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageMemoryBarriers[3].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[3].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[3].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[3].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[4].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[4].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[4].image = ShadowMaskTexture;
		ImageMemoryBarriers[4].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[4].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarriers[4].pNext = nullptr;
		ImageMemoryBarriers[4].srcAccessMask = 0;
		ImageMemoryBarriers[4].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[4].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[4].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[4].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[4].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[4].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[4].subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 5, ImageMemoryBarriers);

		XMMATRIX ReProjMatrices[4];
		ReProjMatrices[0] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[0];
		ReProjMatrices[1] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[1];
		ReProjMatrices[2] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[2];
		ReProjMatrices[3] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[3];

		void *ConstantBufferData;
		size_t ConstantBufferOffset = 0;

		SAFE_VK(vkMapMemory(Device, CPUShadowResolveConstantBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &ConstantBufferData));

		ShadowResolveConstantBuffer& ConstantBuffer = *((ShadowResolveConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.ReProjMatrices[0] = ReProjMatrices[0];
		ConstantBuffer.ReProjMatrices[1] = ReProjMatrices[1];
		ConstantBuffer.ReProjMatrices[2] = ReProjMatrices[2];
		ConstantBuffer.ReProjMatrices[3] = ReProjMatrices[3];

		ConstantBufferOffset += 256;

		vkUnmapMemory(Device, CPUShadowResolveConstantBuffersMemoryHeaps[CurrentFrameIndex]);

		VkBufferMemoryBarrier BufferMemoryBarrier;
		BufferMemoryBarrier.buffer = CPUShadowResolveConstantBuffers[CurrentFrameIndex];
		BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.offset = 0;
		BufferMemoryBarrier.pNext = nullptr;
		BufferMemoryBarrier.size = VK_WHOLE_SIZE;
		BufferMemoryBarrier.srcAccessMask = 0;
		BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

		VkBufferCopy BufferCopy;
		BufferCopy.dstOffset = 0;
		BufferCopy.size = 256;
		BufferCopy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPUShadowResolveConstantBuffers[CurrentFrameIndex], GPUShadowResolveConstantBuffer, 1, &BufferCopy);

		BufferMemoryBarrier.buffer = GPUShadowResolveConstantBuffer;
		BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_UNIFORM_READ_BIT;
		BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.offset = 0;
		BufferMemoryBarrier.pNext = nullptr;
		BufferMemoryBarrier.size = VK_WHOLE_SIZE;
		BufferMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = ShadowMaskFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = ShadowMaskRenderPass;
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

		VkDescriptorBufferInfo DescriptorBufferInfo;
		DescriptorBufferInfo.buffer = GPUShadowResolveConstantBuffer;
		DescriptorBufferInfo.offset = 0;
		DescriptorBufferInfo.range = 256;

		VkDescriptorImageInfo DescriptorImageInfos[6];
		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = ResolvedDepthBufferTextureDepthOnlyView;
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[1].imageView = CascadedShadowMapTexturesViews[0];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[2].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[2].imageView = CascadedShadowMapTexturesViews[1];
		DescriptorImageInfos[2].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[3].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[3].imageView = CascadedShadowMapTexturesViews[2];
		DescriptorImageInfos[3].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[4].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[4].imageView = CascadedShadowMapTexturesViews[3];
		DescriptorImageInfos[4].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[5].imageLayout = (VkImageLayout)0;
		DescriptorImageInfos[5].imageView = VK_NULL_HANDLE;
		DescriptorImageInfos[5].sampler = ShadowMapSampler;

		VkWriteDescriptorSet WriteDescriptorSets[3];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = ShadowResolveSets[CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = &DescriptorBufferInfo;
		WriteDescriptorSets[0].pImageInfo = nullptr;
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 5;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = ShadowResolveSets[CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[2].descriptorCount = 1;
		WriteDescriptorSets[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		WriteDescriptorSets[2].dstArrayElement = 0;
		WriteDescriptorSets[2].dstBinding = 3;
		WriteDescriptorSets[2].dstSet = ShadowResolveSets[CurrentFrameIndex];
		WriteDescriptorSets[2].pBufferInfo = nullptr;
		WriteDescriptorSets[2].pImageInfo = &DescriptorImageInfos[5];
		WriteDescriptorSets[2].pNext = nullptr;
		WriteDescriptorSets[2].pTexelBufferView = nullptr;
		WriteDescriptorSets[2].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 3, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowResolvePipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, ShadowResolvePipelineLayout, 0, 1, &ShadowResolveSets[CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);		
	}

	// ===============================================================================================================	

	VkImageMemoryBarrier ImageMemoryBarriers[1];
	ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
	ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImageMemoryBarriers[0].image = BackBufferTextures[CurrentBackBufferIndex];
	ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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

	vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, ImageMemoryBarriers);

	SAFE_VK(vkEndCommandBuffer(CommandBuffers[CurrentFrameIndex]));

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

	SAFE_VK(vkQueueSubmit(CommandQueue, 1, &SubmitInfo, FrameSyncFences[CurrentFrameIndex]));

	VkPresentInfoKHR PresentInfo;
	PresentInfo.pImageIndices = &CurrentBackBufferIndex;
	PresentInfo.pNext = nullptr;
	PresentInfo.pResults = nullptr;
	PresentInfo.pSwapchains = &SwapChain;
	PresentInfo.pWaitSemaphores = &ImagePresentationSemaphore;
	PresentInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	PresentInfo.swapchainCount = 1;
	PresentInfo.waitSemaphoreCount = 1;

	SAFE_VK(vkQueuePresentKHR(CommandQueue, &PresentInfo));

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
}

RenderMesh* RenderSystem::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMesh *renderMesh = new RenderMesh();

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

	SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &renderMesh->VertexBuffer));

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

		AlignedResourceOffset = 0;
	}

	SAFE_VK(vkBindBufferMemory(Device, renderMesh->VertexBuffer, BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset));

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + MemoryRequirements.size;

	BufferCreateInfo.flags = 0;
	BufferCreateInfo.pNext = nullptr;
	BufferCreateInfo.pQueueFamilyIndices = nullptr;
	BufferCreateInfo.queueFamilyIndexCount = 0;
	BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	BufferCreateInfo.size = sizeof(WORD) * renderMeshCreateInfo.IndexCount;
	BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &renderMesh->IndexBuffer));

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

		AlignedResourceOffset = 0;
	}

	SAFE_VK(vkBindBufferMemory(Device, renderMesh->IndexBuffer, BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset));

	BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] = AlignedResourceOffset + MemoryRequirements.size;
	
	void *MappedData;

	SAFE_VK(vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData));
	memcpy((BYTE*)MappedData, renderMeshCreateInfo.VertexData, sizeof(Vertex) * renderMeshCreateInfo.VertexCount);
	memcpy((BYTE*)MappedData + sizeof(Vertex) * renderMeshCreateInfo.VertexCount, renderMeshCreateInfo.IndexData, sizeof(WORD) * renderMeshCreateInfo.IndexCount);
	vkUnmapMemory(Device, UploadHeap);

	VkCommandBufferBeginInfo CommandBufferBeginInfo;
	CommandBufferBeginInfo.flags = 0;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;
	CommandBufferBeginInfo.pNext = nullptr;
	CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	SAFE_VK(vkBeginCommandBuffer(CommandBuffers[0], &CommandBufferBeginInfo));

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

	SAFE_VK(vkEndCommandBuffer(CommandBuffers[0]));

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

	SAFE_VK(vkQueueSubmit(CommandQueue, 1, &SubmitInfo, CopySyncFence));

	SAFE_VK(vkWaitForFences(Device, 1, &CopySyncFence, VK_FALSE, INFINITE));

	SAFE_VK(vkResetFences(Device, 1, &CopySyncFence));

	return renderMesh;
}

RenderTexture* RenderSystem::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTexture *renderTexture = new RenderTexture();

	VkFormat TextureFormat;

	if (renderTextureCreateInfo.Compressed)
	{
		if (renderTextureCreateInfo.SRGB)
		{
			switch (renderTextureCreateInfo.CompressionType)
			{
			case BlockCompression::BC1:
				TextureFormat = VkFormat::VK_FORMAT_BC1_RGB_SRGB_BLOCK;
				break;
			case BlockCompression::BC2:
				TextureFormat = VkFormat::VK_FORMAT_BC2_SRGB_BLOCK;
				break;
			case BlockCompression::BC3:
				TextureFormat = VkFormat::VK_FORMAT_BC3_SRGB_BLOCK;
				break;
			default:
				TextureFormat = (VkFormat)0;
				break;
			}
		}
		else
		{
			switch (renderTextureCreateInfo.CompressionType)
			{
			case BlockCompression::BC1:
				TextureFormat = VkFormat::VK_FORMAT_BC1_RGB_UNORM_BLOCK;
				break;
			case BlockCompression::BC2:
				TextureFormat = VkFormat::VK_FORMAT_BC2_UNORM_BLOCK;
				break;
			case BlockCompression::BC3:
				TextureFormat = VkFormat::VK_FORMAT_BC3_UNORM_BLOCK;
				break;
			case BlockCompression::BC4:
				TextureFormat = VkFormat::VK_FORMAT_BC4_UNORM_BLOCK;
				break;
			case BlockCompression::BC5:
				TextureFormat = VkFormat::VK_FORMAT_BC5_UNORM_BLOCK;
				break;
			default:
				TextureFormat = (VkFormat)0;
				break;
			}
		}
	}
	else
	{
		if (renderTextureCreateInfo.SRGB)
		{
			TextureFormat = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
		}
		else
		{
			TextureFormat = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
		}
	}

	VkPhysicalDeviceMemoryProperties PhysicalDeviceMemoryProperties;

	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalDeviceMemoryProperties);

	VkImageCreateInfo ImageCreateInfo;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.extent.height = renderTextureCreateInfo.Height;
	ImageCreateInfo.extent.width = renderTextureCreateInfo.Width;
	ImageCreateInfo.flags = 0;
	ImageCreateInfo.format = TextureFormat;
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

	SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &renderTexture->Texture));

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &TextureMemoryHeaps[CurrentTextureMemoryHeapIndex]));

		AlignedResourceOffset = 0;
	}

	SAFE_VK(vkBindImageMemory(Device, renderTexture->Texture, TextureMemoryHeaps[CurrentTextureMemoryHeapIndex], AlignedResourceOffset));

	TextureMemoryHeapOffsets[CurrentTextureMemoryHeapIndex] = AlignedResourceOffset + MemoryRequirements.size;

	void *MappedData;

	SAFE_VK(vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData));

	BYTE *TexelData = renderTextureCreateInfo.TexelData;

	for (UINT i = 0; i < renderTextureCreateInfo.MIPLevels; i++)
	{
		if (renderTextureCreateInfo.Compressed)
		{
			for (UINT j = 0; j < (renderTextureCreateInfo.Height / 4) >> i; j++)
			{
				if (renderTextureCreateInfo.CompressionType == BlockCompression::BC1)
				{
					memcpy((BYTE*)MappedData + j * 8 * ((renderTextureCreateInfo.Width / 4) >> i), (BYTE*)TexelData + j * 8 * ((renderTextureCreateInfo.Width / 4) >> i), 8 * ((renderTextureCreateInfo.Width / 4) >> i));
				}
				else if (renderTextureCreateInfo.CompressionType == BlockCompression::BC5)
				{
					memcpy((BYTE*)MappedData + j * 16 * ((renderTextureCreateInfo.Width / 4) >> i), (BYTE*)TexelData + j * 16 * ((renderTextureCreateInfo.Width / 4) >> i), 16 * ((renderTextureCreateInfo.Width / 4) >> i));
				}
			}
		}
		else
		{
			for (UINT j = 0; j < (renderTextureCreateInfo.Height / 4) >> i; j++)
			{
				memcpy((BYTE*)MappedData + j * 4 * (renderTextureCreateInfo.Width >> i), (BYTE*)TexelData + j * 4 * (renderTextureCreateInfo.Width >> i), 4 * (renderTextureCreateInfo.Width >> i));
			}
		}

		//TexelData += 8 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
		//MappedData = (BYTE*)MappedData + 8 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);

		if (renderTextureCreateInfo.Compressed)
		{
			if (renderTextureCreateInfo.CompressionType == BlockCompression::BC1)
			{
				TexelData += 8 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
				MappedData = (BYTE*)MappedData + 8 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
			}
			else if (renderTextureCreateInfo.CompressionType == BlockCompression::BC5)
			{
				TexelData += 16 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
				MappedData = (BYTE*)MappedData + 16 * ((renderTextureCreateInfo.Width / 4) >> i) * ((renderTextureCreateInfo.Height / 4) >> i);
			}
		}
		else
		{
			TexelData += 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
			MappedData = (BYTE*)MappedData + 4 * (renderTextureCreateInfo.Width >> i) * (renderTextureCreateInfo.Height >> i);
		}
	}

	vkUnmapMemory(Device, UploadHeap);

	VkCommandBufferBeginInfo CommandBufferBeginInfo;
	CommandBufferBeginInfo.flags = 0;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;
	CommandBufferBeginInfo.pNext = nullptr;
	CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	SAFE_VK(vkBeginCommandBuffer(CommandBuffers[0], &CommandBufferBeginInfo));

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
		if (renderTextureCreateInfo.Compressed)
		{
			if (renderTextureCreateInfo.CompressionType == BlockCompression::BC1)
			{
				BufferImageCopies[i].bufferOffset = (i == 0) ? 0 : BufferImageCopies[i - 1].bufferOffset + 8 * ((renderTextureCreateInfo.Width / 4) >> (i - 1)) * ((renderTextureCreateInfo.Height / 4) >> (i - 1));
			}
			else if (renderTextureCreateInfo.CompressionType == BlockCompression::BC5)
			{
				BufferImageCopies[i].bufferOffset = (i == 0) ? 0 : BufferImageCopies[i - 1].bufferOffset + 16 * ((renderTextureCreateInfo.Width / 4) >> (i - 1)) * ((renderTextureCreateInfo.Height / 4) >> (i - 1));
			}
		}
		else
		{
			BufferImageCopies[i].bufferOffset = (i == 0) ? 0 : BufferImageCopies[i - 1].bufferOffset + 4 * (renderTextureCreateInfo.Width >> (i - 1)) * (renderTextureCreateInfo.Height >> (i - 1));
		}

		BufferImageCopies[i].bufferImageHeight = renderTextureCreateInfo.Height >> i;
		BufferImageCopies[i].bufferImageHeight = 0;
		//BufferImageCopies[i].bufferOffset = (i == 0) ? 0 : BufferImageCopies[i - 1].bufferOffset + 8 * ((BufferImageCopies[i - 1].imageExtent.width / 4) * (BufferImageCopies[i - 1].imageExtent.height / 4));
		BufferImageCopies[i].bufferRowLength = renderTextureCreateInfo.Width >> i;
		BufferImageCopies[i].bufferRowLength = 0;
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

	SAFE_VK(vkEndCommandBuffer(CommandBuffers[0]));

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

	SAFE_VK(vkQueueSubmit(CommandQueue, 1, &SubmitInfo, CopySyncFence));

	SAFE_VK(vkWaitForFences(Device, 1, &CopySyncFence, VK_FALSE, INFINITE));

	SAFE_VK(vkResetFences(Device, 1, &CopySyncFence));

	VkImageViewCreateInfo ImageViewCreateInfo;
	ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
	ImageViewCreateInfo.flags = 0;
	ImageViewCreateInfo.format = TextureFormat;
	ImageViewCreateInfo.image = renderTexture->Texture;
	ImageViewCreateInfo.pNext = nullptr;
	ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
	ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	ImageViewCreateInfo.subresourceRange.layerCount = 1;
	ImageViewCreateInfo.subresourceRange.levelCount = renderTextureCreateInfo.MIPLevels;
	ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

	SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &renderTexture->TextureView));

	return renderTexture;
}

RenderMaterial* RenderSystem::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterial *renderMaterial = new RenderMaterial();

	VkShaderModule GBufferOpaquePassVertexShaderModule, GBufferOpaquePassPixelShaderModule;
	VkShaderModule ShadowMapPassVertexShaderModule;

	VkShaderModuleCreateInfo ShaderModuleCreateInfo;
	ShaderModuleCreateInfo.codeSize = renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeLength;
	ShaderModuleCreateInfo.flags = 0;
	ShaderModuleCreateInfo.pCode = (uint32_t*)renderMaterialCreateInfo.GBufferOpaquePassVertexShaderByteCodeData;
	ShaderModuleCreateInfo.pNext = nullptr;
	ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &GBufferOpaquePassVertexShaderModule));

	ShaderModuleCreateInfo.codeSize = renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeLength;
	ShaderModuleCreateInfo.flags = 0;
	ShaderModuleCreateInfo.pCode = (uint32_t*)renderMaterialCreateInfo.GBufferOpaquePassPixelShaderByteCodeData;
	ShaderModuleCreateInfo.pNext = nullptr;
	ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &GBufferOpaquePassPixelShaderModule));

	ShaderModuleCreateInfo.codeSize = renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeLength;
	ShaderModuleCreateInfo.flags = 0;
	ShaderModuleCreateInfo.pCode = (uint32_t*)renderMaterialCreateInfo.ShadowMapPassVertexShaderByteCodeData;
	ShaderModuleCreateInfo.pNext = nullptr;
	ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ShadowMapPassVertexShaderModule));

	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentStates[2];
	ZeroMemory(PipelineColorBlendAttachmentStates, 2 * sizeof(VkPipelineColorBlendAttachmentState));
	PipelineColorBlendAttachmentStates[0].colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
	PipelineColorBlendAttachmentStates[1].colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo;
	ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
	PipelineColorBlendStateCreateInfo.attachmentCount = 2;
	PipelineColorBlendStateCreateInfo.pAttachments = PipelineColorBlendAttachmentStates;
	PipelineColorBlendStateCreateInfo.pNext = nullptr;
	PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo;
	ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
	PipelineDepthStencilStateCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_GREATER;
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

	VkSampleMask SampleMask[2] = { 0xFFFFFFFF, 0xFFFFFFFF };

	VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo;
	PipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.flags = 0;
	PipelineMultisampleStateCreateInfo.minSampleShading = 0.0f;
	PipelineMultisampleStateCreateInfo.pNext = nullptr;
	PipelineMultisampleStateCreateInfo.pSampleMask = SampleMask;
	PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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
	PipelineShaderStageCreateInfos[0].module = GBufferOpaquePassVertexShaderModule;
	PipelineShaderStageCreateInfos[0].pName = "VS";
	PipelineShaderStageCreateInfos[0].pNext = nullptr;
	PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
	PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	PipelineShaderStageCreateInfos[1].flags = 0;
	PipelineShaderStageCreateInfos[1].module = GBufferOpaquePassPixelShaderModule;
	PipelineShaderStageCreateInfos[1].pName = "PS";
	PipelineShaderStageCreateInfos[1].pNext = nullptr;
	PipelineShaderStageCreateInfos[1].pSpecializationInfo = nullptr;
	PipelineShaderStageCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	PipelineShaderStageCreateInfos[1].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	VkVertexInputAttributeDescription VertexInputAttributeDescriptions[5];
	VertexInputAttributeDescriptions[0].binding = 0;
	VertexInputAttributeDescriptions[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[0].location = 0;
	VertexInputAttributeDescriptions[0].offset = 0;
	VertexInputAttributeDescriptions[1].binding = 0;
	VertexInputAttributeDescriptions[1].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
	VertexInputAttributeDescriptions[1].location = 1;
	VertexInputAttributeDescriptions[1].offset = 12;
	VertexInputAttributeDescriptions[2].binding = 0;
	VertexInputAttributeDescriptions[2].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[2].location = 2;
	VertexInputAttributeDescriptions[2].offset = 20;
	VertexInputAttributeDescriptions[3].binding = 0;
	VertexInputAttributeDescriptions[3].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[3].location = 3;
	VertexInputAttributeDescriptions[3].offset = 32;
	VertexInputAttributeDescriptions[4].binding = 0;
	VertexInputAttributeDescriptions[4].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[4].location = 4;
	VertexInputAttributeDescriptions[4].offset = 44;

	VkVertexInputBindingDescription VertexInputBindingDescription;
	VertexInputBindingDescription.binding = 0;
	VertexInputBindingDescription.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
	VertexInputBindingDescription.stride = 56;

	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;
	PipelineVertexInputStateCreateInfo.flags = 0;
	PipelineVertexInputStateCreateInfo.pNext = nullptr;
	PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttributeDescriptions;
	PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &VertexInputBindingDescription;
	PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 5;
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
	GraphicsPipelineCreateInfo.renderPass = GBufferDrawRenderPass;
	GraphicsPipelineCreateInfo.stageCount = 2;
	GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	GraphicsPipelineCreateInfo.subpass = 0;

	SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &renderMaterial->GBufferOpaquePassPipeline));

	ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
	PipelineColorBlendStateCreateInfo.attachmentCount = 0;
	PipelineColorBlendStateCreateInfo.pAttachments = nullptr;
	PipelineColorBlendStateCreateInfo.pNext = nullptr;
	PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
	PipelineDepthStencilStateCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;
	PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	PipelineDepthStencilStateCreateInfo.pNext = nullptr;
	PipelineDepthStencilStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	PipelineDynamicStateCreateInfo.dynamicStateCount = 4;
	PipelineDynamicStateCreateInfo.flags = 0;
	PipelineDynamicStateCreateInfo.pDynamicStates = DynamicStates;
	PipelineDynamicStateCreateInfo.pNext = nullptr;
	PipelineDynamicStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

	PipelineInputAssemblyStateCreateInfo.flags = 0;
	PipelineInputAssemblyStateCreateInfo.pNext = nullptr;
	PipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	PipelineInputAssemblyStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	PipelineInputAssemblyStateCreateInfo.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.flags = 0;
	PipelineMultisampleStateCreateInfo.minSampleShading = 0.0f;
	PipelineMultisampleStateCreateInfo.pNext = nullptr;
	PipelineMultisampleStateCreateInfo.pSampleMask = SampleMask;
	PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
	PipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	PipelineMultisampleStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	ZeroMemory(&PipelineRasterizationStateCreateInfo, sizeof(VkPipelineRasterizationStateCreateInfo));
	PipelineRasterizationStateCreateInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
	PipelineRasterizationStateCreateInfo.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
	PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	PipelineRasterizationStateCreateInfo.pNext = nullptr;
	PipelineRasterizationStateCreateInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
	PipelineRasterizationStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	PipelineShaderStageCreateInfos[0].flags = 0;
	PipelineShaderStageCreateInfos[0].module = ShadowMapPassVertexShaderModule;
	PipelineShaderStageCreateInfos[0].pName = "VS";
	PipelineShaderStageCreateInfos[0].pNext = nullptr;
	PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
	PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

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
	GraphicsPipelineCreateInfo.renderPass = ShadowMapDrawRenderPass;
	GraphicsPipelineCreateInfo.stageCount = 1;
	GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	GraphicsPipelineCreateInfo.subpass = 0;

	SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &renderMaterial->ShadowMapPassPipeline));

	vkDestroyShaderModule(Device, GBufferOpaquePassVertexShaderModule, nullptr);
	vkDestroyShaderModule(Device, GBufferOpaquePassPixelShaderModule, nullptr);

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

inline void RenderSystem::CheckVulkanCallResult(VkResult Result, const char16_t* Function)
{
	if (Result != VkResult::VK_SUCCESS)
	{
		char16_t VulkanErrorMessageBuffer[2048];
		char16_t VulkanErrorCodeBuffer[512];

		const char16_t *VulkanErrorCodePtr = GetVulkanErrorMessageFromVkResult(Result);

		if (VulkanErrorCodePtr) wcscpy((wchar_t*)VulkanErrorCodeBuffer, (const wchar_t*)VulkanErrorCodePtr);
		else wsprintf((wchar_t*)VulkanErrorCodeBuffer, (const wchar_t*)u"0x%08X (неизвестный код)", Result);

		wsprintf((wchar_t*)VulkanErrorMessageBuffer, (const wchar_t*)u"Произошла ошибка при попытке вызова следующей Vulkan-функции:\r\n%s\r\nКод ошибки: %s", (const wchar_t*)Function, (const wchar_t*)VulkanErrorCodeBuffer);

		int IntResult = MessageBox(NULL, (const wchar_t*)VulkanErrorMessageBuffer, (const wchar_t*)u"Ошибка Vulkan", MB_OK | MB_ICONERROR);

		ExitProcess(0);
	}
}

inline const char16_t* RenderSystem::GetVulkanErrorMessageFromVkResult(VkResult Result)
{
	switch (Result)
	{
		case VkResult::VK_ERROR_OUT_OF_HOST_MEMORY:
			return u"VK_ERROR_OUT_OF_HOST_MEMORY";
			break;
		case VkResult::VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return u"VK_ERROR_OUT_OF_DEVICE_MEMORY";
			break;
		case VkResult::VK_ERROR_INITIALIZATION_FAILED:
			return u"VK_ERROR_INITIALIZATION_FAILED";
			break;
		case VkResult::VK_ERROR_DEVICE_LOST:
			return u"VK_ERROR_DEVICE_LOST";
			break;
		case VkResult::VK_ERROR_MEMORY_MAP_FAILED:
			return u"VK_ERROR_MEMORY_MAP_FAILED";
			break;
		case VkResult::VK_ERROR_LAYER_NOT_PRESENT:
			return u"VK_ERROR_LAYER_NOT_PRESENT";
			break;
		case VkResult::VK_ERROR_EXTENSION_NOT_PRESENT:
			return u"VK_ERROR_EXTENSION_NOT_PRESENT";
			break;
		case VkResult::VK_ERROR_FEATURE_NOT_PRESENT:
			return u"VK_ERROR_FEATURE_NOT_PRESENT";
			break;
		case VkResult::VK_ERROR_INCOMPATIBLE_DRIVER:
			return u"VK_ERROR_INCOMPATIBLE_DRIVER";
			break;
		case VkResult::VK_ERROR_TOO_MANY_OBJECTS:
			return u"VK_ERROR_TOO_MANY_OBJECTS";
			break;
		case VkResult::VK_ERROR_FORMAT_NOT_SUPPORTED:
			return u"VK_ERROR_FORMAT_NOT_SUPPORTED";
			break;
		case VkResult::VK_ERROR_FRAGMENTED_POOL:
			return u"VK_ERROR_FRAGMENTED_POOL";
			break;
		case VkResult::VK_ERROR_UNKNOWN:
			return u"VK_ERROR_UNKNOWN";
			break;
		case VkResult::VK_ERROR_OUT_OF_POOL_MEMORY:
			return u"VK_ERROR_OUT_OF_POOL_MEMORY";
			break;
		case VkResult::VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return u"VK_ERROR_INVALID_EXTERNAL_HANDLE";
			break;
		case VkResult::VK_ERROR_FRAGMENTATION:
			return u"VK_ERROR_FRAGMENTATION";
			break;
		case VkResult::VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return u"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
			break;
		case VkResult::VK_ERROR_SURFACE_LOST_KHR:
			return u"VK_ERROR_SURFACE_LOST_KHR";
			break;
		case VkResult::VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return u"VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
			break;
		case VkResult::VK_ERROR_OUT_OF_DATE_KHR:
			return u"VK_ERROR_OUT_OF_DATE_KHR";
			break;
		case VkResult::VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return u"VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
			break;
		case VkResult::VK_ERROR_VALIDATION_FAILED_EXT:
			return u"VK_ERROR_VALIDATION_FAILED_EXT";
			break;
		case VkResult::VK_ERROR_INVALID_SHADER_NV:
			return u"VK_ERROR_INVALID_SHADER_NV";
			break;
		case VkResult::VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return u"VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
			break;
		case VkResult::VK_ERROR_NOT_PERMITTED_EXT:
			return u"VK_ERROR_NOT_PERMITTED_EXT";
			break;
		case VkResult::VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return u"VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
			break;
		case VkResult::VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT:
			return u"VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT";
			break;
		default:
			return nullptr;
			break;
	}

	return nullptr;
}

/*

// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderSystem.h"

#include <Core/Application.h>

#include <Engine/Engine.h>

#include <Game/Components/Common/TransformComponent.h>
#include <Game/Components/Common/BoundingBoxComponent.h>
#include <Game/Components/Render/Meshes/StaticMeshComponent.h>
#include <Game/Components/Render/Lights/PointLightComponent.h>

#include <ResourceManager/Resources/Render/Meshes/StaticMeshResource.h>
#include <ResourceManager/Resources/Render/Materials/MaterialResource.h>
#include <ResourceManager/Resources/Render/Textures/Texture2DResource.h>

struct PointLight
{
	XMFLOAT3 Position;
	float Radius;
	XMFLOAT3 Color;
	float Brightness;
};

struct GBufferOpaquePassConstantBuffer
{
	XMMATRIX WVPMatrix;
	XMMATRIX WorldMatrix;
	XMFLOAT3X4 VectorTransformMatrix;
};

struct ShadowMapPassConstantBuffer
{
	XMMATRIX WVPMatrix;
};

struct ShadowResolveConstantBuffer
{
	XMMATRIX ReProjMatrices[4];
};

struct DeferredLightingConstantBuffer
{
	XMMATRIX InvViewProjMatrix;
	XMFLOAT3 CameraWorldPosition;
};

struct SkyConstantBuffer
{
	XMMATRIX WVPMatrix;
};

struct SunConstantBuffer
{
	XMMATRIX ViewMatrix;
	XMMATRIX ProjMatrix;
	XMFLOAT3 SunPosition;
};

// ===============================================================================================================

HANDLE FullScreenQuadVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/FullScreenQuad.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
LARGE_INTEGER FullScreenQuadVertexShaderByteCodeLength;
BOOL Result = GetFileSizeEx(FullScreenQuadVertexShaderFile, &FullScreenQuadVertexShaderByteCodeLength);
ScopedMemoryBlockArray<BYTE> FullScreenQuadVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FullScreenQuadVertexShaderByteCodeLength.QuadPart);
Result = ReadFile(FullScreenQuadVertexShaderFile, FullScreenQuadVertexShaderByteCodeData, (DWORD)FullScreenQuadVertexShaderByteCodeLength.QuadPart, NULL, NULL);
Result = CloseHandle(FullScreenQuadVertexShaderFile);

// ===============================================================================================================

{
	D3D12_SAMPLER_DESC SamplerDesc;
	ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_ANISOTROPIC;
	SamplerDesc.MaxAnisotropy = 16;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	TextureSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SamplersDescriptorsCount++;

	Device->CreateSampler(&SamplerDesc, TextureSampler);

	ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_LESS;
	SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	ShadowMapSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SamplersDescriptorsCount++;

	Device->CreateSampler(&SamplerDesc, ShadowMapSampler);

	ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	BiLinearSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SamplersDescriptorsCount++;

	Device->CreateSampler(&SamplerDesc, BiLinearSampler);

	ZeroMemory(&SamplerDesc, sizeof(D3D12_SAMPLER_DESC));
	SamplerDesc.AddressU = SamplerDesc.AddressV = SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE::D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc.BorderColor[0] = SamplerDesc.BorderColor[1] = SamplerDesc.BorderColor[2] = SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)0;
	SamplerDesc.Filter = D3D12_FILTER::D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MipLODBias = 0.0f;

	MinSampler.ptr = SamplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + SamplersDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	SamplersDescriptorsCount++;

	Device->CreateSampler(&SamplerDesc, MinSampler);
}

// ===============================================================================================================

{
	D3D12_HEAP_DESC HeapDesc;
	ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
	HeapDesc.Alignment = 0;
	HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapDesc.Properties.CreationNodeMask = 0;
	HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapDesc.Properties.VisibleNodeMask = 0;
	HeapDesc.SizeInBytes = BUFFER_MEMORY_HEAP_SIZE;

	SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(BufferMemoryHeaps[CurrentBufferMemoryHeapIndex])));

	ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
	HeapDesc.Alignment = 0;
	HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapDesc.Properties.CreationNodeMask = 0;
	HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapDesc.Properties.VisibleNodeMask = 0;
	HeapDesc.SizeInBytes = TEXTURE_MEMORY_HEAP_SIZE;

	SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(TextureMemoryHeaps[CurrentTextureMemoryHeapIndex])));

	ZeroMemory(&HeapDesc, sizeof(D3D12_HEAP_DESC));
	HeapDesc.Alignment = 0;
	HeapDesc.Flags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	HeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapDesc.Properties.CreationNodeMask = 0;
	HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapDesc.Properties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapDesc.Properties.VisibleNodeMask = 0;
	HeapDesc.SizeInBytes = UPLOAD_HEAP_SIZE;

	SAFE_DX(Device->CreateHeap(&HeapDesc, UUIDOF(UploadHeap)));

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = UPLOAD_HEAP_SIZE;

	SAFE_DX(Device->CreatePlacedResource(UploadHeap, 0, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(UploadBuffer)));
}

// ===============================================================================================================

{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	ResourceDesc.Height = 2048;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 2048;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[2])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(CascadedShadowMapTextures[3])));

	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Flags = D3D12_DSV_FLAGS::D3D12_DSV_FLAG_NONE;
	DSVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;
	DSVDesc.Texture2D.MipSlice = 0;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION::D3D12_DSV_DIMENSION_TEXTURE2D;

	CascadedShadowMapTexturesDSVs[0].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	DSDescriptorsCount++;
	CascadedShadowMapTexturesDSVs[1].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	DSDescriptorsCount++;
	CascadedShadowMapTexturesDSVs[2].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	DSDescriptorsCount++;
	CascadedShadowMapTexturesDSVs[3].ptr = DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + DSDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	DSDescriptorsCount++;

	Device->CreateDepthStencilView(CascadedShadowMapTextures[0], &DSVDesc, CascadedShadowMapTexturesDSVs[0]);
	Device->CreateDepthStencilView(CascadedShadowMapTextures[1], &DSVDesc, CascadedShadowMapTexturesDSVs[1]);
	Device->CreateDepthStencilView(CascadedShadowMapTextures[2], &DSVDesc, CascadedShadowMapTexturesDSVs[2]);
	Device->CreateDepthStencilView(CascadedShadowMapTextures[3], &DSVDesc, CascadedShadowMapTexturesDSVs[3]);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	CascadedShadowMapTexturesSRVs[0].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;
	CascadedShadowMapTexturesSRVs[1].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;
	CascadedShadowMapTexturesSRVs[2].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;
	CascadedShadowMapTexturesSRVs[3].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(CascadedShadowMapTextures[0], &SRVDesc, CascadedShadowMapTexturesSRVs[0]);
	Device->CreateShaderResourceView(CascadedShadowMapTextures[1], &SRVDesc, CascadedShadowMapTexturesSRVs[1]);
	Device->CreateShaderResourceView(CascadedShadowMapTextures[2], &SRVDesc, CascadedShadowMapTexturesSRVs[2]);
	Device->CreateShaderResourceView(CascadedShadowMapTextures[3], &SRVDesc, CascadedShadowMapTexturesSRVs[3]);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 256 * 20000;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[2])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUConstantBuffers2[3])));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[0][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[1][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[2][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[3][0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[0][1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[1][1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[2][1])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUConstantBuffers2[3][1])));

	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 20000; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			CBVDesc.BufferLocation = GPUConstantBuffers2[j]->GetGPUVirtualAddress() + i * 256;
			CBVDesc.SizeInBytes = 256;

			ConstantBufferCBVs2[j][i].ptr = ConstantBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + ConstantBufferDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			ConstantBufferDescriptorsCount++;

			Device->CreateConstantBufferView(&CBVDesc, ConstantBufferCBVs2[j][i]);
		}
	}
}

// ===============================================================================================================

{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(ShadowMaskTexture)));

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	ShadowMaskTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	RTDescriptorsCount++;

	Device->CreateRenderTargetView(ShadowMaskTexture, &RTVDesc, ShadowMaskTextureRTV);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	ShadowMaskTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(ShadowMaskTexture, &SRVDesc, ShadowMaskTextureSRV);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 256;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUShadowResolveConstantBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUShadowResolveConstantBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUShadowResolveConstantBuffers[1])));

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUShadowResolveConstantBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	ShadowResolveConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateConstantBufferView(&CBVDesc, ShadowResolveConstantBufferCBV);

	HANDLE ShadowResolvePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ShadowResolve.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER ShadowResolvePixelShaderByteCodeLength;
	Result = GetFileSizeEx(ShadowResolvePixelShaderFile, &ShadowResolvePixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> ShadowResolvePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowResolvePixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(ShadowResolvePixelShaderFile, ShadowResolvePixelShaderByteCodeData, (DWORD)ShadowResolvePixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(ShadowResolvePixelShaderFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = ShadowResolvePixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ShadowResolvePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(ShadowResolvePipelineState)));
}

// ===============================================================================================================

{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(HDRSceneColorTexture)));

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	HDRSceneColorTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	RTDescriptorsCount++;

	Device->CreateRenderTargetView(HDRSceneColorTexture, &RTVDesc, HDRSceneColorTextureRTV);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2DMS;

	HDRSceneColorTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(HDRSceneColorTexture, &SRVDesc, HDRSceneColorTextureSRV);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 256;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUDeferredLightingConstantBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUDeferredLightingConstantBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUDeferredLightingConstantBuffers[1])));

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUDeferredLightingConstantBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	DeferredLightingConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateConstantBufferView(&CBVDesc, DeferredLightingConstantBufferCBV);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = sizeof(LightCluster) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPULightClustersBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightClustersBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightClustersBuffers[1])));

	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
	SRVDesc.Buffer.NumElements = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
	SRVDesc.Buffer.StructureByteStride = 0;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

	LightClustersBufferSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(GPULightClustersBuffer, &SRVDesc, LightClustersBufferSRV);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t) * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPULightIndicesBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightIndicesBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPULightIndicesBuffers[1])));

	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
	SRVDesc.Buffer.NumElements = ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z;
	SRVDesc.Buffer.StructureByteStride = 0;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

	LightIndicesBufferSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(GPULightIndicesBuffer, &SRVDesc, LightIndicesBufferSRV);

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 10000 * sizeof(PointLight);

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(GPUPointLightsBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUPointLightsBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUPointLightsBuffers[1])));

	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE;
	SRVDesc.Buffer.NumElements = 10000;
	SRVDesc.Buffer.StructureByteStride = sizeof(PointLight);
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;

	PointLightsBufferSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(GPUPointLightsBuffer, &SRVDesc, PointLightsBufferSRV);

	HANDLE DeferredLightingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/DeferredLighting.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER DeferredLightingPixelShaderByteCodeLength;
	Result = GetFileSizeEx(DeferredLightingPixelShaderFile, &DeferredLightingPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(DeferredLightingPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(DeferredLightingPixelShaderFile, DeferredLightingPixelShaderByteCodeData, (DWORD)DeferredLightingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(DeferredLightingPixelShaderFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = DeferredLightingPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = DeferredLightingPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DeferredLightingPipelineState)));
}

// ===============================================================================================================

{
	UINT SkyMeshVertexCount = 1 + 25 * 100 + 1;
	UINT SkyMeshIndexCount = 300 + 24 * 600 + 300;

	Vertex SkyMeshVertices[1 + 25 * 100 + 1];
	WORD SkyMeshIndices[300 + 24 * 600 + 300];

	SkyMeshVertices[0].Position = XMFLOAT3(0.0f, 1.0f, 0.0f);
	SkyMeshVertices[0].TexCoord = XMFLOAT2(0.5f, 0.5f);

	SkyMeshVertices[1 + 25 * 100].Position = XMFLOAT3(0.0f, -1.0f, 0.0f);
	SkyMeshVertices[1 + 25 * 100].TexCoord = XMFLOAT2(0.5f, 0.5f);

	for (int i = 0; i < 100; i++)
	{
		for (int j = 0; j < 25; j++)
		{
			float Theta = ((j + 1) / 25.0f) * 0.5f * 3.1416f;
			float Phi = (i / 100.0f) * 2.0f * 3.1416f;

			float X = sinf(Theta) * cosf(Phi);
			float Y = sinf(Theta) * sinf(Phi);
			float Z = cosf(Theta);

			SkyMeshVertices[1 + j * 100 + i].Position = XMFLOAT3(X, Z, Y);
			SkyMeshVertices[1 + j * 100 + i].TexCoord = XMFLOAT2(X * 0.5f + 0.5f, Y * 0.5f + 0.5f);
		}
	}

	for (int i = 0; i < 100; i++)
	{
		SkyMeshIndices[3 * i] = 0;
		SkyMeshIndices[3 * i + 1] = i + 1;
		SkyMeshIndices[3 * i + 2] = i != 99 ? i + 2 : 1;
	}

	for (int j = 0; j < 24; j++)
	{
		for (int i = 0; i < 100; i++)
		{
			SkyMeshIndices[300 + j * 600 + 6 * i] = 1 + i + j * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 1] = 1 + i + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 2] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 3] = 1 + i + j * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 4] = i != 99 ? 1 + i + 1 + (j + 1) * 100 : 1 + (j + 1) * 100;
			SkyMeshIndices[300 + j * 600 + 6 * i + 5] = i != 99 ? 1 + i + 1 + j * 100 : 1 + j * 100;
		}
	}

	for (int i = 0; i < 100; i++)
	{
		SkyMeshIndices[300 + 24 * 600 + 3 * i] = 1 + 25 * 100;
		SkyMeshIndices[300 + 24 * 600 + 3 * i + 1] = i != 99 ? 1 + 24 * 100 + i + 1 : 1 + 24 * 100;
		SkyMeshIndices[300 + 24 * 600 + 3 * i + 2] = 1 + 24 * 100 + i;
	}

	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = sizeof(Vertex) * SkyMeshVertexCount;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyVertexBuffer)));

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = sizeof(WORD) * SkyMeshIndexCount;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyIndexBuffer)));

	void *MappedData;

	D3D12_RANGE ReadRange, WrittenRange;
	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * SkyMeshVertexCount;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
	memcpy((BYTE*)MappedData, SkyMeshVertices, sizeof(Vertex) * SkyMeshVertexCount);
	memcpy((BYTE*)MappedData + sizeof(Vertex) * SkyMeshVertexCount, SkyMeshIndices, sizeof(WORD) * SkyMeshIndexCount);
	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CommandAllocators[0]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

	CommandList->CopyBufferRegion(SkyVertexBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * SkyMeshVertexCount);
	CommandList->CopyBufferRegion(SkyIndexBuffer, 0, UploadBuffer, sizeof(Vertex) * SkyMeshVertexCount, sizeof(WORD) * SkyMeshIndexCount);

	D3D12_RESOURCE_BARRIER ResourceBarriers[2];
	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SkyVertexBuffer;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = SkyIndexBuffer;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(2, ResourceBarriers);

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

	if (CopySyncFence->GetCompletedValue() != 1)
	{
		SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
		DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
	}

	SAFE_DX(CopySyncFence->Signal(0));

	SkyVertexBufferAddress = SkyVertexBuffer->GetGPUVirtualAddress();
	SkyIndexBufferAddress = SkyIndexBuffer->GetGPUVirtualAddress();

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 256;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUSkyConstantBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSkyConstantBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSkyConstantBuffers[1])));

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = GPUSkyConstantBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	SkyConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateConstantBufferView(&CBVDesc, SkyConstantBufferCBV);

	HANDLE SkyVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SkyVertexShaderByteCodeLength;
	Result = GetFileSizeEx(SkyVertexShaderFile, &SkyVertexShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SkyVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(SkyVertexShaderFile, SkyVertexShaderByteCodeData, (DWORD)SkyVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SkyVertexShaderFile);

	HANDLE SkyPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SkyPixelShaderByteCodeLength;
	Result = GetFileSizeEx(SkyPixelShaderFile, &SkyPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SkyPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(SkyPixelShaderFile, SkyPixelShaderByteCodeData, (DWORD)SkyPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SkyPixelShaderFile);

	D3D12_INPUT_ELEMENT_DESC InputElementDescs[5];
	ZeroMemory(InputElementDescs, 5 * sizeof(D3D12_INPUT_ELEMENT_DESC));
	InputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 0;
	InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";
	InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[2].InputSlot = 0;
	InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[2].InstanceDataStepRate = 0;
	InputElementDescs[2].SemanticIndex = 0;
	InputElementDescs[2].SemanticName = "NORMAL";
	InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[3].InputSlot = 0;
	InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[3].InstanceDataStepRate = 0;
	InputElementDescs[3].SemanticIndex = 0;
	InputElementDescs[3].SemanticName = "TANGENT";
	InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[4].InputSlot = 0;
	InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[4].InstanceDataStepRate = 0;
	InputElementDescs[4].SemanticIndex = 0;
	InputElementDescs[4].SemanticName = "BINORMAL";

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = SkyPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = SkyPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = SkyVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = SkyVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SkyPipelineState)));

	ScopedMemoryBlockArray<Texel> SkyTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(2048 * 2048);

	for (int y = 0; y < 2048; y++)
	{
		for (int x = 0; x < 2048; x++)
		{
			float X = (x / 2048.0f) * 2.0f - 1.0f;
			float Y = (y / 2048.0f) * 2.0f - 1.0f;

			float D = sqrtf(X * X + Y * Y);

			SkyTextureTexels[y * 2048 + x].R = 0;
			SkyTextureTexels[y * 2048 + x].G = 128 * D < 255 ? BYTE(128 * D) : 255;
			SkyTextureTexels[y * 2048 + x].B = 255;
			SkyTextureTexels[y * 2048 + x].A = 255;
		}
	}

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ResourceDesc.Height = 2048;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 2048;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SkyTexture)));

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedSubResourceFootPrint;

	UINT NumRows;
	UINT64 RowSizeInBytes, TotalBytes;

	Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = TotalBytes;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));

	BYTE *TexelData = (BYTE*)SkyTextureTexels;

	for (UINT j = 0; j < NumRows; j++)
	{
		memcpy((BYTE*)MappedData + PlacedSubResourceFootPrint.Offset + j * PlacedSubResourceFootPrint.Footprint.RowPitch, (BYTE*)TexelData + j * RowSizeInBytes, RowSizeInBytes);
	}

	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CommandAllocators[0]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

	D3D12_TEXTURE_COPY_LOCATION SourceTextureCopyLocation, DestTextureCopyLocation;

	SourceTextureCopyLocation.pResource = UploadBuffer;
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	DestTextureCopyLocation.pResource = SkyTexture;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
	DestTextureCopyLocation.SubresourceIndex = 0;

	CommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

	D3D12_RESOURCE_BARRIER ResourceBarrier;
	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = SkyTexture;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

	if (CopySyncFence->GetCompletedValue() != 1)
	{
		SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
		DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
	}

	SAFE_DX(CopySyncFence->Signal(0));

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	SkyTextureSRV.ptr = TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + TexturesDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	++TexturesDescriptorsCount;

	Device->CreateShaderResourceView(SkyTexture, &SRVDesc, SkyTextureSRV);

	UINT SunMeshVertexCount = 4;
	UINT SunMeshIndexCount = 6;

	Vertex SunMeshVertices[4] = {

		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }

	};

	WORD SunMeshIndices[6] = { 0, 1, 2, 2, 1, 3 };

	COMRCPtr<ID3D12Resource> TemporarySunVertexBuffer, TemporarySunIndexBuffer;

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = sizeof(Vertex) * SunMeshVertexCount;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunVertexBuffer)));

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = sizeof(WORD) * SunMeshIndexCount;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunIndexBuffer)));

	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = sizeof(Vertex) * SunMeshVertexCount + sizeof(WORD) * SunMeshIndexCount;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));
	memcpy((BYTE*)MappedData, SunMeshVertices, sizeof(Vertex) * SunMeshVertexCount);
	memcpy((BYTE*)MappedData + sizeof(Vertex) * SunMeshVertexCount, SunMeshIndices, sizeof(WORD) * SunMeshIndexCount);
	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CommandAllocators[0]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

	CommandList->CopyBufferRegion(SunVertexBuffer, 0, UploadBuffer, 0, sizeof(Vertex) * SunMeshVertexCount);
	CommandList->CopyBufferRegion(SunIndexBuffer, 0, UploadBuffer, sizeof(Vertex) * SunMeshVertexCount, sizeof(WORD) * SunMeshIndexCount);

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = SunVertexBuffer;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = SunIndexBuffer;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(2, ResourceBarriers);

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

	if (CopySyncFence->GetCompletedValue() != 1)
	{
		SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
		DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
	}

	SAFE_DX(CopySyncFence->Signal(0));

	SunVertexBufferAddress = SunVertexBuffer->GetGPUVirtualAddress();
	SunIndexBufferAddress = SunIndexBuffer->GetGPUVirtualAddress();

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 256;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, UUIDOF(GPUSunConstantBuffer)));

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSunConstantBuffers[0])));
	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, UUIDOF(CPUSunConstantBuffers[1])));

	CBVDesc.BufferLocation = GPUSunConstantBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = 256;

	SunConstantBufferCBV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateConstantBufferView(&CBVDesc, SunConstantBufferCBV);

	HANDLE SunVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunVertexShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SunVertexShaderByteCodeLength;
	Result = GetFileSizeEx(SunVertexShaderFile, &SunVertexShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SunVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunVertexShaderByteCodeLength.QuadPart);
	Result = ReadFile(SunVertexShaderFile, SunVertexShaderByteCodeData, (DWORD)SunVertexShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SunVertexShaderFile);

	HANDLE SunPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunPixelShader.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER SunPixelShaderByteCodeLength;
	Result = GetFileSizeEx(SunPixelShaderFile, &SunPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> SunPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(SunPixelShaderFile, SunPixelShaderByteCodeData, (DWORD)SunPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(SunPixelShaderFile);

	ZeroMemory(InputElementDescs, 5 * sizeof(D3D12_INPUT_ELEMENT_DESC));
	InputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[0].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[0].InputSlot = 0;
	InputElementDescs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[0].InstanceDataStepRate = 0;
	InputElementDescs[0].SemanticIndex = 0;
	InputElementDescs[0].SemanticName = "POSITION";
	InputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[1].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
	InputElementDescs[1].InputSlot = 0;
	InputElementDescs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[1].InstanceDataStepRate = 0;
	InputElementDescs[1].SemanticIndex = 0;
	InputElementDescs[1].SemanticName = "TEXCOORD";
	InputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[2].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[2].InputSlot = 0;
	InputElementDescs[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[2].InstanceDataStepRate = 0;
	InputElementDescs[2].SemanticIndex = 0;
	InputElementDescs[2].SemanticName = "NORMAL";
	InputElementDescs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[3].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[3].InputSlot = 0;
	InputElementDescs[3].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[3].InstanceDataStepRate = 0;
	InputElementDescs[3].SemanticIndex = 0;
	InputElementDescs[3].SemanticName = "TANGENT";
	InputElementDescs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	InputElementDescs[4].Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
	InputElementDescs[4].InputSlot = 0;
	InputElementDescs[4].InputSlotClass = D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	InputElementDescs[4].InstanceDataStepRate = 0;
	InputElementDescs[4].SemanticIndex = 0;
	InputElementDescs[4].SemanticName = "BINORMAL";

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.DepthStencilState.DepthEnable = TRUE;
	GraphicsPipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER;
	GraphicsPipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
	GraphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 5;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = InputElementDescs;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = SunPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = SunPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = SunVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = SunVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(SunPipelineState)));

	ScopedMemoryBlockArray<Texel> SunTextureTexels = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<Texel>(512 * 512);

	for (int y = 0; y < 512; y++)
	{
		for (int x = 0; x < 512; x++)
		{
			float X = (x / 512.0f) * 2.0f - 1.0f;
			float Y = (y / 512.0f) * 2.0f - 1.0f;

			float D = sqrtf(X * X + Y * Y);

			SunTextureTexels[y * 512 + x].R = 255;
			SunTextureTexels[y * 512 + x].G = 255;
			SunTextureTexels[y * 512 + x].B = 127 + 128 * D < 255 ? BYTE(127 + 128 * D) : 255;
			SunTextureTexels[y * 512 + x].A = 255 * D < 255 ? BYTE(255 * D) : 255;
		}
	}

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ResourceDesc.Height = 512;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 512;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, UUIDOF(SunTexture)));

	Device->GetCopyableFootprints(&ResourceDesc, 0, 1, 0, &PlacedSubResourceFootPrint, &NumRows, &RowSizeInBytes, &TotalBytes);

	ReadRange.Begin = 0;
	ReadRange.End = 0;

	WrittenRange.Begin = 0;
	WrittenRange.End = TotalBytes;

	SAFE_DX(UploadBuffer->Map(0, &ReadRange, &MappedData));

	TexelData = (BYTE*)SunTextureTexels;

	for (UINT j = 0; j < NumRows; j++)
	{
		memcpy((BYTE*)MappedData + PlacedSubResourceFootPrint.Offset + j * PlacedSubResourceFootPrint.Footprint.RowPitch, (BYTE*)TexelData + j * RowSizeInBytes, RowSizeInBytes);
	}

	UploadBuffer->Unmap(0, &WrittenRange);

	SAFE_DX(CommandAllocators[0]->Reset());
	SAFE_DX(CommandList->Reset(CommandAllocators[0], nullptr));

	SourceTextureCopyLocation.pResource = UploadBuffer;
	SourceTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	DestTextureCopyLocation.pResource = SunTexture;
	DestTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE::D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	SourceTextureCopyLocation.PlacedFootprint = PlacedSubResourceFootPrint;
	DestTextureCopyLocation.SubresourceIndex = 0;

	CommandList->CopyTextureRegion(&DestTextureCopyLocation, 0, 0, 0, &SourceTextureCopyLocation, nullptr);

	ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarrier.Transition.pResource = SunTexture;
	ResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
	ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, &ResourceBarrier);

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(CommandQueue->Signal(CopySyncFence, 1));

	if (CopySyncFence->GetCompletedValue() != 1)
	{
		SAFE_DX(CopySyncFence->SetEventOnCompletion(1, CopySyncEvent));
		DWORD WaitResult = WaitForSingleObject(CopySyncEvent, INFINITE);
	}

	SAFE_DX(CopySyncFence->Signal(0));

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	SunTextureSRV.ptr = TexturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + TexturesDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	++TexturesDescriptorsCount;

	Device->CreateShaderResourceView(SunTexture, &SRVDesc, SunTextureSRV);

	HANDLE FogPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/Fog.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER FogPixelShaderByteCodeLength;
	Result = GetFileSizeEx(FogPixelShaderFile, &FogPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> FogPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FogPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(FogPixelShaderFile, FogPixelShaderByteCodeData, (DWORD)FogPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(FogPixelShaderFile);

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = FogPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = FogPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(FogPipelineState)));
}

// ===============================================================================================================

{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(ResolvedHDRSceneColorTexture)));

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	ResolvedHDRSceneColorTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(ResolvedHDRSceneColorTexture, &SRVDesc, ResolvedHDRSceneColorTextureSRV);
}

// ===============================================================================================================

{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	//ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	//ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	int Widths[4] = { 1280, 80, 5, 1 };
	int Heights[4] = { 720, 45, 3, 1 };

	for (int i = 0; i < 4; i++)
	{
		ResourceDesc.Width = Widths[i];
		ResourceDesc.Height = Heights[i];

		SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr, UUIDOF(SceneLuminanceTextures[i])));
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SceneLuminanceTexturesUAVs[i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateUnorderedAccessView(SceneLuminanceTextures[i], nullptr, &UAVDesc, SceneLuminanceTexturesUAVs[i]);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 4; i++)
	{
		SceneLuminanceTexturesSRVs[i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(SceneLuminanceTextures[i], &SRVDesc, SceneLuminanceTexturesSRVs[i]);
	}

	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	ResourceDesc.Height = 1;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = 1;

	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, UUIDOF(AverageLuminanceTexture)));

	UAVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	UAVDesc.Texture2D.MipSlice = 0;
	UAVDesc.Texture2D.PlaneSlice = 0;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureUAV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateUnorderedAccessView(AverageLuminanceTexture, nullptr, &UAVDesc, AverageLuminanceTextureUAV);

	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	AverageLuminanceTextureSRV.ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CBSRUADescriptorsCount++;

	Device->CreateShaderResourceView(AverageLuminanceTexture, &SRVDesc, AverageLuminanceTextureSRV);

	HANDLE LuminanceCalcComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceCalc.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceCalcComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceCalcComputeShaderFile, &LuminanceCalcComputeShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceCalcComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceCalcComputeShaderFile, LuminanceCalcComputeShaderByteCodeData, (DWORD)LuminanceCalcComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceCalcComputeShaderFile);

	HANDLE LuminanceSumComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceSum.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceSumComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceSumComputeShaderFile, &LuminanceSumComputeShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceSumComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceSumComputeShaderFile, LuminanceSumComputeShaderByteCodeData, (DWORD)LuminanceSumComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceSumComputeShaderFile);

	HANDLE LuminanceAvgComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceAvg.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER LuminanceAvgComputeShaderByteCodeLength;
	Result = GetFileSizeEx(LuminanceAvgComputeShaderFile, &LuminanceAvgComputeShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceAvgComputeShaderByteCodeLength.QuadPart);
	Result = ReadFile(LuminanceAvgComputeShaderFile, LuminanceAvgComputeShaderByteCodeData, (DWORD)LuminanceAvgComputeShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(LuminanceAvgComputeShaderFile);

	D3D12_COMPUTE_PIPELINE_STATE_DESC ComputePipelineStateDesc;
	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceCalcComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceCalcComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

	SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceCalcPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceSumComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceSumComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

	SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceSumPipelineState)));

	ZeroMemory(&ComputePipelineStateDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
	ComputePipelineStateDesc.CS.BytecodeLength = LuminanceAvgComputeShaderByteCodeLength.QuadPart;
	ComputePipelineStateDesc.CS.pShaderBytecode = LuminanceAvgComputeShaderByteCodeData;
	ComputePipelineStateDesc.pRootSignature = ComputeRootSignature;

	SAFE_DX(Device->CreateComputePipelineState(&ComputePipelineStateDesc, UUIDOF(LuminanceAvgPipelineState)));
}

// ===============================================================================================================

{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	//ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	//ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

	for (int i = 0; i < 7; i++)
	{
		ResourceDesc.Width = ResolutionWidth >> i;
		ResourceDesc.Height = ResolutionHeight >> i;

		SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(BloomTextures[0][i])));
		SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(BloomTextures[1][i])));
		SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &ClearValue, UUIDOF(BloomTextures[2][i])));
	}

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	RTVDesc.Texture2D.MipSlice = 0;
	RTVDesc.Texture2D.PlaneSlice = 0;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 7; i++)
	{
		BloomTexturesRTVs[0][i].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;
		BloomTexturesRTVs[1][i].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;
		BloomTexturesRTVs[2][i].ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		RTDescriptorsCount++;

		Device->CreateRenderTargetView(BloomTextures[0][i], &RTVDesc, BloomTexturesRTVs[0][i]);
		Device->CreateRenderTargetView(BloomTextures[1][i], &RTVDesc, BloomTexturesRTVs[1][i]);
		Device->CreateRenderTargetView(BloomTextures[2][i], &RTVDesc, BloomTexturesRTVs[2][i]);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.PlaneSlice = 0;
	SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < 7; i++)
	{
		BloomTexturesSRVs[0][i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;
		BloomTexturesSRVs[1][i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;
		BloomTexturesSRVs[2][i].ptr = CBSRUADescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + CBSRUADescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		CBSRUADescriptorsCount++;

		Device->CreateShaderResourceView(BloomTextures[0][i], &SRVDesc, BloomTexturesSRVs[0][i]);
		Device->CreateShaderResourceView(BloomTextures[1][i], &SRVDesc, BloomTexturesSRVs[1][i]);
		Device->CreateShaderResourceView(BloomTextures[2][i], &SRVDesc, BloomTexturesSRVs[2][i]);
	}

	HANDLE BrightPassPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/BrightPass.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER BrightPassPixelShaderByteCodeLength;
	Result = GetFileSizeEx(BrightPassPixelShaderFile, &BrightPassPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> BrightPassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(BrightPassPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(BrightPassPixelShaderFile, BrightPassPixelShaderByteCodeData, (DWORD)BrightPassPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(BrightPassPixelShaderFile);

	HANDLE ImageResamplePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ImageResample.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER ImageResamplePixelShaderByteCodeLength;
	Result = GetFileSizeEx(ImageResamplePixelShaderFile, &ImageResamplePixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> ImageResamplePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ImageResamplePixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(ImageResamplePixelShaderFile, ImageResamplePixelShaderByteCodeData, (DWORD)ImageResamplePixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(ImageResamplePixelShaderFile);

	HANDLE HorizontalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HorizontalBlur.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER HorizontalBlurPixelShaderByteCodeLength;
	Result = GetFileSizeEx(HorizontalBlurPixelShaderFile, &HorizontalBlurPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> HorizontalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HorizontalBlurPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(HorizontalBlurPixelShaderFile, HorizontalBlurPixelShaderByteCodeData, (DWORD)HorizontalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(HorizontalBlurPixelShaderFile);

	HANDLE VerticalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/VerticalBlur.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER VerticalBlurPixelShaderByteCodeLength;
	Result = GetFileSizeEx(VerticalBlurPixelShaderFile, &VerticalBlurPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> VerticalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(VerticalBlurPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(VerticalBlurPixelShaderFile, VerticalBlurPixelShaderByteCodeData, (DWORD)VerticalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(VerticalBlurPixelShaderFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = BrightPassPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = BrightPassPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(BrightPassPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = ImageResamplePixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ImageResamplePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(DownSamplePipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND::D3D12_BLEND_ONE;
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND::D3D12_BLEND_ZERO;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = ImageResamplePixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = ImageResamplePixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(UpSampleWithAddBlendPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = HorizontalBlurPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = HorizontalBlurPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(HorizontalBlurPipelineState)));

	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = VerticalBlurPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = VerticalBlurPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
	GraphicsPipelineStateDesc.SampleDesc.Count = 1;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(VerticalBlurPipelineState)));
}

// ===============================================================================================================

{
	D3D12_RESOURCE_DESC ResourceDesc;
	ZeroMemory(&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	ResourceDesc.Alignment = 0;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ResourceDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ResourceDesc.Height = ResolutionHeight;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.SampleDesc.Count = 8;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width = ResolutionWidth;

	D3D12_HEAP_PROPERTIES HeapProperties;
	ZeroMemory(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.Type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	HeapProperties.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE ClearValue;
	ClearValue.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ClearValue.Color[0] = 0.0f;
	ClearValue.Color[1] = 0.0f;
	ClearValue.Color[2] = 0.0f;
	ClearValue.Color[3] = 0.0f;

	SAFE_DX(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE, &ClearValue, UUIDOF(ToneMappedImageTexture)));

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RTVDesc.ViewDimension = D3D12_RTV_DIMENSION::D3D12_RTV_DIMENSION_TEXTURE2DMS;

	ToneMappedImageTextureRTV.ptr = RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + RTDescriptorsCount * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	RTDescriptorsCount++;

	Device->CreateRenderTargetView(ToneMappedImageTexture, &RTVDesc, ToneMappedImageTextureRTV);

	HANDLE HDRToneMappingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HDRToneMapping.dxbc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	LARGE_INTEGER HDRToneMappingPixelShaderByteCodeLength;
	Result = GetFileSizeEx(HDRToneMappingPixelShaderFile, &HDRToneMappingPixelShaderByteCodeLength);
	ScopedMemoryBlockArray<BYTE> HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HDRToneMappingPixelShaderByteCodeLength.QuadPart);
	Result = ReadFile(HDRToneMappingPixelShaderFile, HDRToneMappingPixelShaderByteCodeData, (DWORD)HDRToneMappingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
	Result = CloseHandle(HDRToneMappingPixelShaderFile);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC GraphicsPipelineStateDesc;
	ZeroMemory(&GraphicsPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	GraphicsPipelineStateDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE::D3D12_COLOR_WRITE_ENABLE_ALL;
	GraphicsPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAGS::D3D12_PIPELINE_STATE_FLAG_NONE;
	GraphicsPipelineStateDesc.InputLayout.NumElements = 0;
	GraphicsPipelineStateDesc.InputLayout.pInputElementDescs = nullptr;
	GraphicsPipelineStateDesc.NodeMask = 0;
	GraphicsPipelineStateDesc.NumRenderTargets = 1;
	GraphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	GraphicsPipelineStateDesc.pRootSignature = GraphicsRootSignature;
	GraphicsPipelineStateDesc.PS.BytecodeLength = HDRToneMappingPixelShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.PS.pShaderBytecode = HDRToneMappingPixelShaderByteCodeData;
	GraphicsPipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	GraphicsPipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE::D3D12_FILL_MODE_SOLID;
	GraphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	GraphicsPipelineStateDesc.SampleDesc.Count = 8;
	GraphicsPipelineStateDesc.SampleDesc.Quality = 0;
	GraphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	GraphicsPipelineStateDesc.VS.BytecodeLength = FullScreenQuadVertexShaderByteCodeLength.QuadPart;
	GraphicsPipelineStateDesc.VS.pShaderBytecode = FullScreenQuadVertexShaderByteCodeData;

	SAFE_DX(Device->CreateGraphicsPipelineState(&GraphicsPipelineStateDesc, UUIDOF(HDRToneMappingPipelineState)));
}


void RenderSystem::TickSystem(float DeltaTime)
{
	GameFramework& gameFramework = Engine::GetEngine().GetGameFramework();

	Camera& camera = gameFramework.GetCamera();

	XMMATRIX ViewMatrix = camera.GetViewMatrix();
	XMMATRIX ProjMatrix = camera.GetProjMatrix();
	XMMATRIX ViewProjMatrix = camera.GetViewProjMatrix();

	XMFLOAT3 CameraLocation = camera.GetCameraLocation();

	XMMATRIX ShadowViewMatrices[4], ShadowProjMatrices[4], ShadowViewProjMatrices[4];

	ShadowViewMatrices[0] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 10.0f, CameraLocation.y + 10.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[1] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 20.0f, CameraLocation.y + 20.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[2] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 50.0f, CameraLocation.y + 50.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	ShadowViewMatrices[3] = XMMatrixLookToLH(XMVectorSet(CameraLocation.x - 100.0f, CameraLocation.y + 100.0f, CameraLocation.z - 10.0f, 1.0f), XMVectorSet(1.0f, -1.0f, 1.0f, 0.0f), XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

	ShadowProjMatrices[0] = XMMatrixOrthographicLH(10.0f, 10.0f, 0.01f, 500.0f);
	ShadowProjMatrices[1] = XMMatrixOrthographicLH(20.0f, 20.0f, 0.01f, 500.0f);
	ShadowProjMatrices[2] = XMMatrixOrthographicLH(50.0f, 50.0f, 0.01f, 500.0f);
	ShadowProjMatrices[3] = XMMatrixOrthographicLH(100.0f, 100.0f, 0.01f, 500.0f);

	ShadowViewProjMatrices[0] = ShadowViewMatrices[0] * ShadowProjMatrices[0];
	ShadowViewProjMatrices[1] = ShadowViewMatrices[1] * ShadowProjMatrices[1];
	ShadowViewProjMatrices[2] = ShadowViewMatrices[2] * ShadowProjMatrices[2];
	ShadowViewProjMatrices[3] = ShadowViewMatrices[3] * ShadowProjMatrices[3];

	RenderScene& renderScene = gameFramework.GetWorld().GetRenderScene();

	vector<StaticMeshComponent*> AllStaticMeshComponents = renderScene.GetStaticMeshComponents();
	vector<StaticMeshComponent*> VisbleStaticMeshComponents = cullingSubSystem.GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.size();

	vector<PointLightComponent*> AllPointLightComponents = renderScene.GetPointLightComponents();
	vector<PointLightComponent*> VisblePointLightComponents = cullingSubSystem.GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

	clusterizationSubSystem.ClusterizeLights(VisblePointLightComponents, ViewMatrix);

	vector<PointLight> PointLights;

	for (PointLightComponent *pointLightComponent : VisblePointLightComponents)
	{
		PointLight pointLight;
		pointLight.Brightness = pointLightComponent->GetBrightness();
		pointLight.Color = pointLightComponent->GetColor();
		pointLight.Position = pointLightComponent->GetTransformComponent()->GetLocation();
		pointLight.Radius = pointLightComponent->GetRadius();

		PointLights.push_back(pointLight);
	}


	// ===============================================================================================================

	D3D12_RESOURCE_BARRIER ResourceBarriers[8];
	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = GBufferTextures[0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = GBufferTextures[1];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = DepthBufferTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ResolvedDepthBufferTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ResolvedDepthBufferTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = CascadedShadowMapTextures[0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = CascadedShadowMapTextures[1];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = CascadedShadowMapTextures[2];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[3].Transition.pResource = CascadedShadowMapTextures[3];
	ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[3].Transition.Subresource = 0;
	ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ShadowMaskTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = CascadedShadowMapTextures[0];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = CascadedShadowMapTextures[1];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[3].Transition.pResource = CascadedShadowMapTextures[2];
	ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[3].Transition.Subresource = 0;
	ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[4].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[4].Transition.pResource = CascadedShadowMapTextures[3];
	ResourceBarriers[4].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[4].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
	ResourceBarriers[4].Transition.Subresource = 0;
	ResourceBarriers[4].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		XMMATRIX ReProjMatrices[4];
		ReProjMatrices[0] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[0];
		ReProjMatrices[1] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[1];
		ReProjMatrices[2] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[2];
		ReProjMatrices[3] = XMMatrixInverse(nullptr, ViewProjMatrix) * ShadowViewProjMatrices[3];

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUShadowResolveConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		ShadowResolveConstantBuffer& ConstantBuffer = *((ShadowResolveConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.ReProjMatrices[0] = ReProjMatrices[0];
		ConstantBuffer.ReProjMatrices[1] = ReProjMatrices[1];
		ConstantBuffer.ReProjMatrices[2] = ReProjMatrices[2];
		ConstantBuffer.ReProjMatrices[3] = ReProjMatrices[3];

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUShadowResolveConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		ResourceBarriers[5].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[5].Transition.pResource = GPUShadowResolveConstantBuffer;
		ResourceBarriers[5].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[5].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[5].Transition.Subresource = 0;
		ResourceBarriers[5].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(6, ResourceBarriers);

		CommandList->CopyBufferRegion(GPUShadowResolveConstantBuffer, 0, CPUShadowResolveConstantBuffers[CurrentFrameIndex], 0, 256);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUShadowResolveConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &ShadowMaskTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(ShadowMaskTexture, nullptr);

		Device->CopyDescriptorsSimple(1, SamplerCPUHandle, ShadowMapSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		SamplerCPUHandle.ptr += SamplerHandleSize;

		CommandList->SetGraphicsRootDescriptorTable(5, D3D12_GPU_DESCRIPTOR_HANDLE{ SamplerGPUHandle.ptr + 0 * ResourceHandleSize });
		SamplerGPUHandle.ptr += SamplerHandleSize;

		UINT DestRangeSize = 6;
		UINT SourceRangeSizes[7] = { 1, 1, 4 };
		D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[7] = { ShadowResolveConstantBufferCBV, ResolvedDepthBufferTextureSRV, CascadedShadowMapTexturesSRVs[0] };

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 3, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 6 * ResourceHandleSize;

		CommandList->SetPipelineState(ShadowResolvePipelineState);

		CommandList->SetGraphicsRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 6 * ResourceHandleSize;

		CommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = GBufferTextures[0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = GBufferTextures[1];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = HDRSceneColorTexture;
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[3].Transition.pResource = ShadowMaskTexture;
	ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[3].Transition.Subresource = 0;
	ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUDeferredLightingConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

		DeferredLightingConstantBuffer& ConstantBuffer = *((DeferredLightingConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.InvViewProjMatrix = InvViewProjMatrix;
		ConstantBuffer.CameraWorldPosition = CameraLocation;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUDeferredLightingConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		void *DynamicBufferData;

		SAFE_DX(CPULightClustersBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, clusterizationSubSystem.GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

		WrittenRange.Begin = 0;
		WrittenRange.End = 32 * 18 * 24 * 2 * sizeof(uint32_t);

		CPULightClustersBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		SAFE_DX(CPULightIndicesBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, clusterizationSubSystem.GetLightIndicesData(), clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t));

		WrittenRange.Begin = 0;
		WrittenRange.End = clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t);

		CPULightIndicesBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		SAFE_DX(CPUPointLightsBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &DynamicBufferData));

		memcpy(DynamicBufferData, PointLights.data(), PointLights.size() * sizeof(PointLight));

		WrittenRange.Begin = 0;
		WrittenRange.End = PointLights.size() * sizeof(PointLight);

		CPUPointLightsBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		ResourceBarriers[4].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[4].Transition.pResource = GPUDeferredLightingConstantBuffer;
		ResourceBarriers[4].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[4].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[4].Transition.Subresource = 0;
		ResourceBarriers[4].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[5].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[5].Transition.pResource = GPULightClustersBuffer;
		ResourceBarriers[5].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[5].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[5].Transition.Subresource = 0;
		ResourceBarriers[5].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[6].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[6].Transition.pResource = GPULightIndicesBuffer;
		ResourceBarriers[6].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[6].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[6].Transition.Subresource = 0;
		ResourceBarriers[6].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[7].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[7].Transition.pResource = GPUPointLightsBuffer;
		ResourceBarriers[7].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[7].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[7].Transition.Subresource = 0;
		ResourceBarriers[7].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(8, ResourceBarriers);

		CommandList->CopyBufferRegion(GPUDeferredLightingConstantBuffer, 0, CPUDeferredLightingConstantBuffers[CurrentFrameIndex], 0, 256);
		CommandList->CopyBufferRegion(GPULightClustersBuffer, 0, CPULightClustersBuffers[CurrentFrameIndex], 0, ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * sizeof(LightCluster));
		CommandList->CopyBufferRegion(GPULightIndicesBuffer, 0, CPULightIndicesBuffers[CurrentFrameIndex], 0, clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t));
		CommandList->CopyBufferRegion(GPUPointLightsBuffer, 0, CPUPointLightsBuffers[CurrentFrameIndex], 0, PointLights.size() * sizeof(PointLight));

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUDeferredLightingConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = GPULightClustersBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[2].Transition.pResource = GPULightIndicesBuffer;
		ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[2].Transition.Subresource = 0;
		ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[3].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[3].Transition.pResource = GPUPointLightsBuffer;
		ResourceBarriers[3].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[3].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[3].Transition.Subresource = 0;
		ResourceBarriers[3].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(4, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(HDRSceneColorTexture, nullptr);

		UINT DestRangeSize = 8;
		UINT SourceRangeSizes[7] = { 1, 2, 1, 1, 1, 1, 1 };
		D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[7] = { DeferredLightingConstantBufferCBV, GBufferTexturesSRVs[0], DepthBufferTextureSRV, ShadowMaskTextureSRV, LightClustersBufferSRV, LightIndicesBufferSRV, PointLightsBufferSRV };

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 7, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 8 * ResourceHandleSize;

		CommandList->SetPipelineState(DeferredLightingPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 8 * ResourceHandleSize;

		CommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	{
		XMMATRIX SkyWorldMatrix = XMMatrixScaling(900.0f, 900.0f, 900.0f) * XMMatrixTranslation(CameraLocation.x, CameraLocation.y, CameraLocation.z);
		XMMATRIX SkyWVPMatrix = SkyWorldMatrix * ViewProjMatrix;

		D3D12_RANGE ReadRange, WrittenRange;
		ReadRange.Begin = 0;
		ReadRange.End = 0;

		void *ConstantBufferData;

		SAFE_DX(CPUSkyConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		SkyConstantBuffer& skyConstantBuffer = *((SkyConstantBuffer*)((BYTE*)ConstantBufferData));

		skyConstantBuffer.WVPMatrix = SkyWVPMatrix;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUSkyConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);

		SAFE_DX(CPUSunConstantBuffers[CurrentFrameIndex]->Map(0, &ReadRange, &ConstantBufferData));

		SunConstantBuffer& sunConstantBuffer = *((SunConstantBuffer*)((BYTE*)ConstantBufferData));

		sunConstantBuffer.ViewMatrix = ViewMatrix;
		sunConstantBuffer.ProjMatrix = ProjMatrix;
		sunConstantBuffer.SunPosition = SunPosition;

		WrittenRange.Begin = 0;
		WrittenRange.End = 256;

		CPUSunConstantBuffers[CurrentFrameIndex]->Unmap(0, &WrittenRange);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUSkyConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = GPUSunConstantBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->CopyBufferRegion(GPUSkyConstantBuffer, 0, CPUSkyConstantBuffers[CurrentFrameIndex], 0, 256);
		CommandList->CopyBufferRegion(GPUSunConstantBuffer, 0, CPUSunConstantBuffers[CurrentFrameIndex], 0, 256);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = GPUSkyConstantBuffer;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = GPUSunConstantBuffer;
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		UINT DestRangeSize = 1;
		UINT SourceRangeSizes[2] = { 1 };
		D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { DepthBufferTextureSRV };

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

		CommandList->SetPipelineState(FogPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

		CommandList->DrawInstanced(4, 1, 0, 0);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = DepthBufferTexture;
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		CommandList->OMSetRenderTargets(1, &HDRSceneColorTextureRTV, TRUE, &DepthBufferTextureDSV);

		Viewport.Height = float(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = float(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		Device->CopyDescriptorsSimple(1, SamplerCPUHandle, TextureSampler, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		SamplerCPUHandle.ptr += SamplerHandleSize;

		CommandList->SetGraphicsRootDescriptorTable(5, D3D12_GPU_DESCRIPTOR_HANDLE{ SamplerGPUHandle.ptr + 0 * ResourceHandleSize });
		SamplerGPUHandle.ptr += SamplerHandleSize;

		DestRangeSize = 2;
		SourceRangeSizes[0] = 1;
		SourceRangeSizes[1] = 1;
		SourceCPUHandles[0] = SkyConstantBufferCBV;
		SourceCPUHandles[1] = SkyTextureSRV;

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		VertexBufferView.BufferLocation = SkyVertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * (1 + 25 * 100 + 1);
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		IndexBufferView.BufferLocation = SkyIndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * (300 + 24 * 600 + 300);

		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		CommandList->IASetIndexBuffer(&IndexBufferView);

		CommandList->SetPipelineState(SkyPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(0, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->DrawIndexedInstanced(300 + 24 * 600 + 300, 1, 0, 0, 0);

		DestRangeSize = 2;
		SourceRangeSizes[0] = 1;
		SourceRangeSizes[1] = 1;
		SourceCPUHandles[0] = SunConstantBufferCBV;
		SourceCPUHandles[1] = SunTextureSRV;

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		VertexBufferView.BufferLocation = SunVertexBufferAddress;
		VertexBufferView.SizeInBytes = sizeof(Vertex) * 4;
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		IndexBufferView.BufferLocation = SunIndexBufferAddress;
		IndexBufferView.Format = DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
		IndexBufferView.SizeInBytes = sizeof(WORD) * 6;

		CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		CommandList->IASetIndexBuffer(&IndexBufferView);

		CommandList->SetPipelineState(SunPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(0, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = HDRSceneColorTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ResolvedHDRSceneColorTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->ResolveSubresource(ResolvedHDRSceneColorTexture, 0, HDRSceneColorTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = HDRSceneColorTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ResolvedHDRSceneColorTexture;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[2].Transition.pResource = SceneLuminanceTextures[0];
	ResourceBarriers[2].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	ResourceBarriers[2].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[2].Transition.Subresource = 0;
	ResourceBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(3, ResourceBarriers);

		UINT DestRangeSize = 2;
		UINT SourceRangeSizes[2] = { 1, 1 };
		D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { ResolvedHDRSceneColorTextureSRV, SceneLuminanceTexturesUAVs[0] };

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->SetPipelineState(LuminanceCalcPipelineState);

		CommandList->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		CommandList->DiscardResource(SceneLuminanceTextures[0], nullptr);

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->Dispatch(80, 45, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[1];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		DestRangeSize = 2;
		SourceRangeSizes[0] = 1;
		SourceRangeSizes[1] = 1;
		SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[0];
		SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[1];

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->SetPipelineState(LuminanceSumPipelineState);

		CommandList->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		CommandList->DiscardResource(SceneLuminanceTextures[1], nullptr);

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->Dispatch(80, 45, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[1];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[2];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		DestRangeSize = 2;
		SourceRangeSizes[0] = 1;
		SourceRangeSizes[1] = 1;
		SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[1];
		SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[2];

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		CommandList->DiscardResource(SceneLuminanceTextures[2], nullptr);

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->Dispatch(5, 3, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[2];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = SceneLuminanceTextures[3];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		DestRangeSize = 2;
		SourceRangeSizes[0] = 1;
		SourceRangeSizes[1] = 1;
		SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[2];
		SourceCPUHandles[1] = SceneLuminanceTexturesUAVs[3];

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		CommandList->DiscardResource(SceneLuminanceTextures[3], nullptr);

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->Dispatch(1, 1, 1);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = SceneLuminanceTextures[3];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		DestRangeSize = 2;
		SourceRangeSizes[0] = 1;
		SourceRangeSizes[1] = 1;
		SourceCPUHandles[0] = SceneLuminanceTexturesSRVs[3];
		SourceCPUHandles[1] = AverageLuminanceTextureUAV;

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->SetPipelineState(LuminanceAvgPipelineState);

		CommandList->SetComputeRootDescriptorTable(1, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });
		CommandList->SetComputeRootDescriptorTable(3, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 1 * ResourceHandleSize });

		CommandList->DiscardResource(AverageLuminanceTexture, nullptr);

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->Dispatch(1, 1, 1);
	}

	// ===============================================================================================================

	{
		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = BloomTextures[0][0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(1, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[0][0], TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(BloomTextures[0][0], nullptr);

		UINT DestRangeSize = 2;
		UINT SourceRangeSizes[2] = { 1, 1 };
		D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { ResolvedHDRSceneColorTextureSRV, SceneLuminanceTexturesSRVs[0] };

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->SetPipelineState(BrightPassPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->DrawInstanced(4, 1, 0, 0);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = BloomTextures[0][0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = BloomTextures[1][0];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[1][0], TRUE, nullptr);

		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(BloomTextures[1][0], nullptr);

		DestRangeSize = 1;
		SourceRangeSizes[0] = 1;
		SourceCPUHandles[0] = BloomTexturesSRVs[0][0];

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

		CommandList->SetPipelineState(HorizontalBlurPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

		CommandList->DrawInstanced(4, 1, 0, 0);

		ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[0].Transition.pResource = BloomTextures[1][0];
		ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[0].Transition.Subresource = 0;
		ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
		ResourceBarriers[1].Transition.pResource = BloomTextures[2][0];
		ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
		ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		ResourceBarriers[1].Transition.Subresource = 0;
		ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][0], TRUE, nullptr);

		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(BloomTextures[2][0], nullptr);

		DestRangeSize = 1;
		SourceRangeSizes[0] = 1;
		SourceCPUHandles[0] = BloomTexturesSRVs[1][0];

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

		CommandList->SetPipelineState(VerticalBlurPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

		CommandList->DrawInstanced(4, 1, 0, 0);

		for (int i = 1; i < 7; i++)
		{
			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[0][i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(1, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[0][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			CommandList->DiscardResource(BloomTextures[0][i], nullptr);

			DestRangeSize = 1;
			SourceRangeSizes[0] = 1;
			SourceCPUHandles[0] = BloomTexturesSRVs[0][i - 1];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->SetPipelineState(DownSamplePipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->DrawInstanced(4, 1, 0, 0);

			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[0][i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[1].Transition.pResource = BloomTextures[1][i];
			ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[1].Transition.Subresource = 0;
			ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(2, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[1][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			CommandList->DiscardResource(BloomTextures[1][i], nullptr);

			DestRangeSize = 1;
			SourceRangeSizes[0] = 1;
			SourceCPUHandles[0] = BloomTexturesSRVs[0][i];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->SetPipelineState(HorizontalBlurPipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->DrawInstanced(4, 1, 0, 0);

			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[1][i];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[1].Transition.pResource = BloomTextures[2][i];
			ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[1].Transition.Subresource = 0;
			ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(2, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			CommandList->DiscardResource(BloomTextures[2][i], nullptr);

			DestRangeSize = 1;
			SourceRangeSizes[0] = 1;
			SourceCPUHandles[0] = BloomTexturesSRVs[1][i];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->SetPipelineState(VerticalBlurPipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->DrawInstanced(4, 1, 0, 0);
		}

		for (int i = 5; i >= 0; i--)
		{
			ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
			ResourceBarriers[0].Transition.pResource = BloomTextures[2][i + 1];
			ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
			ResourceBarriers[0].Transition.Subresource = 0;
			ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

			CommandList->ResourceBarrier(1, ResourceBarriers);

			CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			CommandList->OMSetRenderTargets(1, &BloomTexturesRTVs[2][i], TRUE, nullptr);

			Viewport.Height = FLOAT(ResolutionHeight >> i);
			Viewport.MaxDepth = 1.0f;
			Viewport.MinDepth = 0.0f;
			Viewport.TopLeftX = 0.0f;
			Viewport.TopLeftY = 0.0f;
			Viewport.Width = FLOAT(ResolutionWidth >> i);

			CommandList->RSSetViewports(1, &Viewport);

			ScissorRect.bottom = ResolutionHeight >> i;
			ScissorRect.left = 0;
			ScissorRect.right = ResolutionWidth >> i;
			ScissorRect.top = 0;

			CommandList->RSSetScissorRects(1, &ScissorRect);

			DestRangeSize = 1;
			SourceRangeSizes[0] = 1;
			SourceCPUHandles[0] = BloomTexturesSRVs[2][i + 1];

			Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 1, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			ResourceCPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->SetPipelineState(UpSampleWithAddBlendPipelineState);

			CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

			ResourceGPUHandle.ptr += 1 * ResourceHandleSize;

			CommandList->DrawInstanced(4, 1, 0, 0);
		}
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = BloomTextures[2][0];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = ToneMappedImageTexture;
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		CommandList->OMSetRenderTargets(1, &ToneMappedImageTextureRTV, TRUE, nullptr);

		D3D12_VIEWPORT Viewport;
		Viewport.Height = FLOAT(ResolutionHeight);
		Viewport.MaxDepth = 1.0f;
		Viewport.MinDepth = 0.0f;
		Viewport.TopLeftX = 0.0f;
		Viewport.TopLeftY = 0.0f;
		Viewport.Width = FLOAT(ResolutionWidth);

		CommandList->RSSetViewports(1, &Viewport);

		D3D12_RECT ScissorRect;
		ScissorRect.bottom = ResolutionHeight;
		ScissorRect.left = 0;
		ScissorRect.right = ResolutionWidth;
		ScissorRect.top = 0;

		CommandList->RSSetScissorRects(1, &ScissorRect);

		CommandList->DiscardResource(ToneMappedImageTexture, nullptr);

		UINT DestRangeSize = 2;
		UINT SourceRangeSizes[2] = { 1, 1 };
		D3D12_CPU_DESCRIPTOR_HANDLE SourceCPUHandles[2] = { HDRSceneColorTextureSRV, BloomTexturesSRVs[2][0] };

		Device->CopyDescriptors(1, &ResourceCPUHandle, &DestRangeSize, 2, SourceCPUHandles, SourceRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceCPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->SetPipelineState(HDRToneMappingPipelineState);

		CommandList->SetGraphicsRootDescriptorTable(4, D3D12_GPU_DESCRIPTOR_HANDLE{ ResourceGPUHandle.ptr + 0 * ResourceHandleSize });

		ResourceGPUHandle.ptr += 2 * ResourceHandleSize;

		CommandList->DrawInstanced(4, 1, 0, 0);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = ToneMappedImageTexture;
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	ResourceBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[1].Transition.pResource = BackBufferTextures[CurrentBackBufferIndex];
	ResourceBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarriers[1].Transition.Subresource = 0;
	ResourceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	// ===============================================================================================================

	{
		CommandList->ResourceBarrier(2, ResourceBarriers);

		CommandList->ResolveSubresource(BackBufferTextures[CurrentBackBufferIndex], 0, ToneMappedImageTexture, 0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	}

	// ===============================================================================================================

	ResourceBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ResourceBarriers[0].Transition.pResource = BackBufferTextures[CurrentBackBufferIndex];
	ResourceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;
	ResourceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST;
	ResourceBarriers[0].Transition.Subresource = 0;
	ResourceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	CommandList->ResourceBarrier(1, ResourceBarriers);

	// ===============================================================================================================

	SAFE_DX(CommandList->Close());

	CommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&CommandList);

	SAFE_DX(SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));

	SAFE_DX(CommandQueue->Signal(FrameSyncFences[CurrentFrameIndex], 1));

	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;
	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

*/