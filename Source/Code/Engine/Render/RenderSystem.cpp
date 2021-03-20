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
	clusterizationSubSystem.PreComputeClustersPlanes();

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
	EnabledPhysicalDeviceFeatures.sampleRateShading = VK_TRUE;

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

	uint32_t EnabledDeviceExtensionsCount = 6;

	const char* EnabledDeviceExtensionsNames[] = 
	{ 
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MULTIVIEW_EXTENSION_NAME, 
		VK_KHR_MAINTENANCE2_EXTENSION_NAME, 
		VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, 
		VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
		VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME
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

	VkDescriptorPoolSize DescriptorPoolSizes[6];
	DescriptorPoolSizes[0].descriptorCount = 100000 + 4 + 2;
	DescriptorPoolSizes[0].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	DescriptorPoolSizes[1].descriptorCount = 40000 + 1 + 5 + 4 + 1 + 2 + 5 + 54 + 2;
	DescriptorPoolSizes[1].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	DescriptorPoolSizes[2].descriptorCount = 1 + 1 + 1 + 2 + 27;
	DescriptorPoolSizes[2].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
	DescriptorPoolSizes[3].descriptorCount = 2;
	DescriptorPoolSizes[3].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
	DescriptorPoolSizes[4].descriptorCount = 1;
	DescriptorPoolSizes[4].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	DescriptorPoolSizes[5].descriptorCount = 5;
	DescriptorPoolSizes[5].type = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

	VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
	DescriptorPoolCreateInfo.flags = 0;
	DescriptorPoolCreateInfo.maxSets = 100000 + 20000 + 1 + 1 + 1 + 1 + 1 + 2 + 5 + 27 + 1;
	DescriptorPoolCreateInfo.pNext = nullptr;
	DescriptorPoolCreateInfo.poolSizeCount = 6;
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
		SamplerCreateInfo.pNext = nullptr;
		SamplerCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		SAFE_VK(vkCreateSampler(Device, &SamplerCreateInfo, nullptr, &BiLinearSampler));
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
		ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

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

	// ===============================================================================================================	

	{
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
		ImageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		ImageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT;

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &HDRSceneColorTexture));

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, HDRSceneColorTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &HDRSceneColorTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, HDRSceneColorTexture, HDRSceneColorTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		ImageViewCreateInfo.image = HDRSceneColorTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &HDRSceneColorTextureView));

		VkAttachmentDescription AttachmentDescription;
		AttachmentDescription.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.flags = 0;
		AttachmentDescription.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		AttachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &DeferredLightingRenderPass));

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = &HDRSceneColorTextureView;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = DeferredLightingRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &HDRSceneColorFrameBuffer));

		VkBufferCreateInfo BufferCreateInfo;
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = 256;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUDeferredLightingConstantBuffer));

		vkGetBufferMemoryRequirements(Device, GPUDeferredLightingConstantBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUDeferredLightingConstantBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPUDeferredLightingConstantBuffer, GPUDeferredLightingConstantBufferMemoryHeap, 0));

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUDeferredLightingConstantBuffers[0]));

		vkGetBufferMemoryRequirements(Device, CPUDeferredLightingConstantBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUDeferredLightingConstantBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPUDeferredLightingConstantBuffers[0], CPUDeferredLightingConstantBuffersMemoryHeaps[0], 0));

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUDeferredLightingConstantBuffers[1]));

		vkGetBufferMemoryRequirements(Device, CPUDeferredLightingConstantBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUDeferredLightingConstantBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPUDeferredLightingConstantBuffers[1], CPUDeferredLightingConstantBuffersMemoryHeaps[1], 0));
		
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * 2 * sizeof(uint32_t);
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPULightClustersBuffer));

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPULightClustersBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPULightClustersBuffers[1]));

		vkGetBufferMemoryRequirements(Device, GPULightClustersBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPULightClustersBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPULightClustersBuffer, GPULightClustersBufferMemoryHeap, 0));

		vkGetBufferMemoryRequirements(Device, CPULightClustersBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPULightClustersBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPULightClustersBuffers[0], CPULightClustersBuffersMemoryHeaps[0], 0));

		vkGetBufferMemoryRequirements(Device, CPULightClustersBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPULightClustersBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPULightClustersBuffers[1], CPULightClustersBuffersMemoryHeaps[1], 0));

		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t);
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPULightIndicesBuffer));

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPULightIndicesBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPULightIndicesBuffers[1]));

		vkGetBufferMemoryRequirements(Device, GPULightIndicesBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPULightIndicesBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPULightIndicesBuffer, GPULightIndicesBufferMemoryHeap, 0));

		vkGetBufferMemoryRequirements(Device, CPULightIndicesBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPULightIndicesBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPULightIndicesBuffers[0], CPULightIndicesBuffersMemoryHeaps[0], 0));

		vkGetBufferMemoryRequirements(Device, CPULightIndicesBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPULightIndicesBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPULightIndicesBuffers[1], CPULightIndicesBuffersMemoryHeaps[1], 0));

		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = 10000 * 2 * 4 * sizeof(float);
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUPointLightsBuffer));

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUPointLightsBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUPointLightsBuffers[1]));

		vkGetBufferMemoryRequirements(Device, GPUPointLightsBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUPointLightsBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPUPointLightsBuffer, GPUPointLightsBufferMemoryHeap, 0));

		vkGetBufferMemoryRequirements(Device, CPUPointLightsBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUPointLightsBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPUPointLightsBuffers[0], CPUPointLightsBuffersMemoryHeaps[0], 0));

		vkGetBufferMemoryRequirements(Device, CPUPointLightsBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUPointLightsBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPUPointLightsBuffers[1], CPUPointLightsBuffersMemoryHeaps[1], 0));

		VkBufferViewCreateInfo BufferViewCreateInfo;
		BufferViewCreateInfo.buffer = GPULightClustersBuffer;
		BufferViewCreateInfo.flags = 0;
		BufferViewCreateInfo.format = VkFormat::VK_FORMAT_R32G32_UINT;
		BufferViewCreateInfo.offset = 0;
		BufferViewCreateInfo.pNext = nullptr;
		BufferViewCreateInfo.range = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * 2 * sizeof(uint32_t);
		BufferViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;

		SAFE_VK(vkCreateBufferView(Device, &BufferViewCreateInfo, nullptr, &LightClustersBufferView));

		BufferViewCreateInfo.buffer = GPULightIndicesBuffer;
		BufferViewCreateInfo.flags = 0;
		BufferViewCreateInfo.format = VkFormat::VK_FORMAT_R16_UINT;
		BufferViewCreateInfo.offset = 0;
		BufferViewCreateInfo.pNext = nullptr;
		BufferViewCreateInfo.range = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * ClusterizationSubSystem::MAX_LIGHTS_PER_CLUSTER * sizeof(uint16_t);
		BufferViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;

		SAFE_VK(vkCreateBufferView(Device, &BufferViewCreateInfo, nullptr, &LightIndicesBufferView));

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[8];
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
		DescriptorSetLayoutBindings[2].descriptorCount = 1;
		DescriptorSetLayoutBindings[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[2].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[3].binding = 3;
		DescriptorSetLayoutBindings[3].descriptorCount = 1;
		DescriptorSetLayoutBindings[3].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[3].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[3].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[4].binding = 4;
		DescriptorSetLayoutBindings[4].descriptorCount = 1;
		DescriptorSetLayoutBindings[4].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[4].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[4].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[5].binding = 5;
		DescriptorSetLayoutBindings[5].descriptorCount = 1;
		DescriptorSetLayoutBindings[5].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		DescriptorSetLayoutBindings[5].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[5].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[6].binding = 6;
		DescriptorSetLayoutBindings[6].descriptorCount = 1;
		DescriptorSetLayoutBindings[6].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		DescriptorSetLayoutBindings[6].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[6].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[7].binding = 7;
		DescriptorSetLayoutBindings[7].descriptorCount = 1;
		DescriptorSetLayoutBindings[7].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		DescriptorSetLayoutBindings[7].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[7].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
		DescriptorSetLayoutCreateInfo.bindingCount = 8;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &DeferredLightingSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &DeferredLightingSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &DeferredLightingPipelineLayout));

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &DeferredLightingSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &DeferredLightingSets[0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &DeferredLightingSets[1]));

		VkShaderModule DeferredLightingShaderModule;

		HANDLE DeferredLightingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/DeferredLighting.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER DeferredLightingPixelShaderByteCodeLength;
		BOOL Result = GetFileSizeEx(DeferredLightingPixelShaderFile, &DeferredLightingPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(DeferredLightingPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(DeferredLightingPixelShaderFile, DeferredLightingPixelShaderByteCodeData, (DWORD)DeferredLightingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(DeferredLightingPixelShaderFile);

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = DeferredLightingPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = DeferredLightingPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &DeferredLightingShaderModule));

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
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		PipelineMultisampleStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		PipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_TRUE;
		PipelineMultisampleStateCreateInfo.minSampleShading = 1.0f;

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
		PipelineShaderStageCreateInfos[1].module = DeferredLightingShaderModule;
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
		GraphicsPipelineCreateInfo.layout = DeferredLightingPipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = DeferredLightingRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &DeferredLightingPipeline));
	}

	// ===============================================================================================================	

	{
		VkAttachmentDescription AttachmentDescriptions[2];
		AttachmentDescriptions[0].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].flags = 0;
		AttachmentDescriptions[0].format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		AttachmentDescriptions[0].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[0].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[0].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[0].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[0].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[1].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].flags = 0;
		AttachmentDescriptions[1].format = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;
		AttachmentDescriptions[1].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[1].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[1].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
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

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &SkyAndSunRenderPass));

		VkImageView Attachments[2] = { HDRSceneColorTextureView, DepthBufferTextureView };

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 2;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = Attachments;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = SkyAndSunRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &HDRSceneColorAndDepthFrameBuffer));

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[3];
		DescriptorSetLayoutBindings[0].binding = 0;
		DescriptorSetLayoutBindings[0].descriptorCount = 1;
		DescriptorSetLayoutBindings[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		DescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[0].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		DescriptorSetLayoutBindings[1].binding = 1;
		DescriptorSetLayoutBindings[1].descriptorCount = 1;
		DescriptorSetLayoutBindings[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[1].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		DescriptorSetLayoutBindings[2].binding = 2;
		DescriptorSetLayoutBindings[2].descriptorCount = 1;
		DescriptorSetLayoutBindings[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		DescriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[2].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
		DescriptorSetLayoutCreateInfo.bindingCount = 3;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &SkyAndSunSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &SkyAndSunSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &SkyAndSunPipelineLayout));

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &SkyAndSunSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SkyAndSunSets[0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SkyAndSunSets[1][0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SkyAndSunSets[0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &SkyAndSunSets[1][1]));

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

		UINT SunMeshVertexCount = 4;
		UINT SunMeshIndexCount = 6;

		Vertex SunMeshVertices[4] = {

			{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }

		};

		WORD SunMeshIndices[6] = { 0, 1, 2, 2, 1, 3 };

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

		VkBufferCreateInfo BufferCreateInfo;
		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = sizeof(Vertex) * SkyMeshVertexCount;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &SkyVertexBuffer));

		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = sizeof(WORD) * SkyMeshIndexCount;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &SkyIndexBuffer));

		VkMemoryRequirements MemoryRequirements;

		vkGetBufferMemoryRequirements(Device, SkyVertexBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &SkyVertexBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, SkyVertexBuffer, SkyVertexBufferMemoryHeap, 0));

		vkGetBufferMemoryRequirements(Device, SkyIndexBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &SkyIndexBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, SkyIndexBuffer, SkyIndexBufferMemoryHeap, 0));

		void *MappedData;

		SAFE_VK(vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData));
		memcpy((BYTE*)MappedData, SkyMeshVertices, sizeof(Vertex) * SkyMeshVertexCount);
		memcpy((BYTE*)MappedData + sizeof(Vertex) * SkyMeshVertexCount, SkyMeshIndices, sizeof(WORD) * SkyMeshIndexCount);
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
		BufferCopy.size = sizeof(Vertex) * SkyMeshVertexCount;
		BufferCopy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffers[0], UploadBuffer, SkyVertexBuffer, 1, &BufferCopy);

		BufferCopy.dstOffset = 0;
		BufferCopy.size = sizeof(WORD) * SkyMeshIndexCount;
		BufferCopy.srcOffset = sizeof(Vertex) * SkyMeshVertexCount;

		vkCmdCopyBuffer(CommandBuffers[0], UploadBuffer, SkyIndexBuffer, 1, &BufferCopy);

		BufferMemoryBarriers[0].buffer = SkyVertexBuffer;
		BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].offset = 0;
		BufferMemoryBarriers[0].pNext = nullptr;
		BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[1].buffer = SkyIndexBuffer;
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

		SAFE_VK(vkWaitForFences(Device, 1, &CopySyncFence, VK_FALSE, UINT64_MAX));

		SAFE_VK(vkResetFences(Device, 1, &CopySyncFence));

		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = sizeof(Vertex) * SunMeshVertexCount;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &SunVertexBuffer));

		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = sizeof(WORD) * SunMeshIndexCount;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &SunIndexBuffer));

		vkGetBufferMemoryRequirements(Device, SunVertexBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &SunVertexBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, SunVertexBuffer, SunVertexBufferMemoryHeap, 0));

		vkGetBufferMemoryRequirements(Device, SunIndexBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &SunIndexBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, SunIndexBuffer, SunIndexBufferMemoryHeap, 0));

		SAFE_VK(vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData));
		memcpy((BYTE*)MappedData, SunMeshVertices, sizeof(Vertex) * SunMeshVertexCount);
		memcpy((BYTE*)MappedData + sizeof(Vertex) * SunMeshVertexCount, SunMeshIndices, sizeof(WORD) * SunMeshIndexCount);
		vkUnmapMemory(Device, UploadHeap);

		CommandBufferBeginInfo.flags = 0;
		CommandBufferBeginInfo.pInheritanceInfo = nullptr;
		CommandBufferBeginInfo.pNext = nullptr;
		CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		SAFE_VK(vkBeginCommandBuffer(CommandBuffers[0], &CommandBufferBeginInfo));

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

		BufferCopy.dstOffset = 0;
		BufferCopy.size = sizeof(Vertex) * SunMeshVertexCount;
		BufferCopy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffers[0], UploadBuffer, SunVertexBuffer, 1, &BufferCopy);

		BufferCopy.dstOffset = 0;
		BufferCopy.size = sizeof(WORD) * SunMeshIndexCount;
		BufferCopy.srcOffset = sizeof(Vertex) * SunMeshVertexCount;

		vkCmdCopyBuffer(CommandBuffers[0], UploadBuffer, SunIndexBuffer, 1, &BufferCopy);

		BufferMemoryBarriers[0].buffer = SunVertexBuffer;
		BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].offset = 0;
		BufferMemoryBarriers[0].pNext = nullptr;
		BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[1].buffer = SunIndexBuffer;
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

		SAFE_VK(vkWaitForFences(Device, 1, &CopySyncFence, VK_FALSE, UINT64_MAX));

		SAFE_VK(vkResetFences(Device, 1, &CopySyncFence));
		
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.extent.height = 2048;
		ImageCreateInfo.extent.width = 2048;
		ImageCreateInfo.flags = 0;
		ImageCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
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
		ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &SkyTexture));

		ImageCreateInfo.extent.height = 512;
		ImageCreateInfo.extent.width = 512;

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &SunTexture));

		vkGetImageMemoryRequirements(Device, SkyTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &SkyTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, SkyTexture, SkyTextureMemoryHeap, 0));

		vkGetImageMemoryRequirements(Device, SunTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &SunTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, SunTexture, SunTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_SRGB;
		ImageViewCreateInfo.image = SkyTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SkyTextureView));

		ImageViewCreateInfo.image = SunTexture;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SunTextureView));

		SAFE_VK(vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData));

		for (int i = 0; i < 2048; i++)
		{
			memcpy(((BYTE*)MappedData) + i * 2048 * 4, (BYTE*)SkyTextureTexels + i * 2048 * 4, 2048 * 4);
		}

		vkUnmapMemory(Device, UploadHeap);

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
		ImageMemoryBarrier.image = SkyTexture;
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
		ImageMemoryBarrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 1, &ImageMemoryBarrier);

		VkBufferImageCopy BufferImageCopy;
		BufferImageCopy.bufferImageHeight = 2048;
		BufferImageCopy.bufferOffset = 0;
		BufferImageCopy.bufferRowLength = 2048;
		BufferImageCopy.imageExtent.depth = 1;
		BufferImageCopy.imageExtent.height = 2048;
		BufferImageCopy.imageExtent.width = 2048;
		BufferImageCopy.imageOffset.x = 0;
		BufferImageCopy.imageOffset.y = 0;
		BufferImageCopy.imageOffset.z = 0;
		BufferImageCopy.imageSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		BufferImageCopy.imageSubresource.baseArrayLayer = 0;
		BufferImageCopy.imageSubresource.layerCount = 1;
		BufferImageCopy.imageSubresource.mipLevel = 0;

		vkCmdCopyBufferToImage(CommandBuffers[0], UploadBuffer, SkyTexture, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

		ImageMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.image = SkyTexture;
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
		ImageMemoryBarrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrier);

		SAFE_VK(vkEndCommandBuffer(CommandBuffers[0]));

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

		SAFE_VK(vkWaitForFences(Device, 1, &CopySyncFence, VK_FALSE, UINT64_MAX));

		SAFE_VK(vkResetFences(Device, 1, &CopySyncFence));

		SAFE_VK(vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData));

		for (int i = 0; i < 512; i++)
		{
			memcpy(((BYTE*)MappedData) + i * 512 * 4, (BYTE*)SunTextureTexels + i * 512 * 4, 512 * 4);
		}

		vkUnmapMemory(Device, UploadHeap);

		CommandBufferBeginInfo.flags = 0;
		CommandBufferBeginInfo.pInheritanceInfo = nullptr;
		CommandBufferBeginInfo.pNext = nullptr;
		CommandBufferBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		SAFE_VK(vkBeginCommandBuffer(CommandBuffers[0], &CommandBufferBeginInfo));

		BufferMemoryBarrier.buffer = UploadBuffer;
		BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.offset = 0;
		BufferMemoryBarrier.pNext = nullptr;
		BufferMemoryBarrier.size = VK_WHOLE_SIZE;
		BufferMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
		BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		ImageMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.image = SunTexture;
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
		ImageMemoryBarrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 1, &ImageMemoryBarrier);

		BufferImageCopy.bufferImageHeight = 512;
		BufferImageCopy.bufferOffset = 0;
		BufferImageCopy.bufferRowLength = 512;
		BufferImageCopy.imageExtent.depth = 1;
		BufferImageCopy.imageExtent.height = 512;
		BufferImageCopy.imageExtent.width = 512;
		BufferImageCopy.imageOffset.x = 0;
		BufferImageCopy.imageOffset.y = 0;
		BufferImageCopy.imageOffset.z = 0;
		BufferImageCopy.imageSubresource.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		BufferImageCopy.imageSubresource.baseArrayLayer = 0;
		BufferImageCopy.imageSubresource.layerCount = 1;
		BufferImageCopy.imageSubresource.mipLevel = 0;

		vkCmdCopyBufferToImage(CommandBuffers[0], UploadBuffer, SunTexture, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

		ImageMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.image = SunTexture;
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
		ImageMemoryBarrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrier);

		SAFE_VK(vkEndCommandBuffer(CommandBuffers[0]));

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

		SAFE_VK(vkWaitForFences(Device, 1, &CopySyncFence, VK_FALSE, UINT64_MAX));

		SAFE_VK(vkResetFences(Device, 1, &CopySyncFence));

		BufferCreateInfo.flags = 0;
		BufferCreateInfo.pNext = nullptr;
		BufferCreateInfo.pQueueFamilyIndices = nullptr;
		BufferCreateInfo.queueFamilyIndexCount = 0;
		BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
		BufferCreateInfo.size = 256;
		BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUSkyConstantBuffer));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &GPUSunConstantBuffer));

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUSkyConstantBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUSkyConstantBuffers[1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUSunConstantBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUSunConstantBuffers[1]));

		vkGetBufferMemoryRequirements(Device, GPUSkyConstantBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUSkyConstantBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPUSkyConstantBuffer, GPUSkyConstantBufferMemoryHeap, 0));

		vkGetBufferMemoryRequirements(Device, CPUSkyConstantBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUSkyConstantBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPUSkyConstantBuffers[0], CPUSkyConstantBuffersMemoryHeaps[0], 0));

		vkGetBufferMemoryRequirements(Device, CPUSkyConstantBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUSkyConstantBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPUSkyConstantBuffers[1], CPUSkyConstantBuffersMemoryHeaps[1], 0));

		vkGetBufferMemoryRequirements(Device, GPUSunConstantBuffer, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUSunConstantBufferMemoryHeap));

		SAFE_VK(vkBindBufferMemory(Device, GPUSunConstantBuffer, GPUSunConstantBufferMemoryHeap, 0));

		vkGetBufferMemoryRequirements(Device, CPUSunConstantBuffers[0], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUSunConstantBuffersMemoryHeaps[0]));

		SAFE_VK(vkBindBufferMemory(Device, CPUSunConstantBuffers[0], CPUSunConstantBuffersMemoryHeaps[0], 0));

		vkGetBufferMemoryRequirements(Device, CPUSunConstantBuffers[1], &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUSunConstantBuffersMemoryHeaps[1]));

		SAFE_VK(vkBindBufferMemory(Device, CPUSunConstantBuffers[1], CPUSunConstantBuffersMemoryHeaps[1], 0));

		HANDLE SkyVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyVertexShader.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SkyVertexShaderByteCodeLength;
		Result = GetFileSizeEx(SkyVertexShaderFile, &SkyVertexShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SkyVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyVertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(SkyVertexShaderFile, SkyVertexShaderByteCodeData, (DWORD)SkyVertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SkyVertexShaderFile);

		HANDLE SkyPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SkyPixelShader.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SkyPixelShaderByteCodeLength;
		Result = GetFileSizeEx(SkyPixelShaderFile, &SkyPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SkyPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(SkyPixelShaderFile, SkyPixelShaderByteCodeData, (DWORD)SkyPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SkyPixelShaderFile);

		HANDLE SunVertexShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunVertexShader.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SunVertexShaderByteCodeLength;
		Result = GetFileSizeEx(SunVertexShaderFile, &SunVertexShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SunVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunVertexShaderByteCodeLength.QuadPart);
		Result = ReadFile(SunVertexShaderFile, SunVertexShaderByteCodeData, (DWORD)SunVertexShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SunVertexShaderFile);

		HANDLE SunPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/SunPixelShader.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER SunPixelShaderByteCodeLength;
		Result = GetFileSizeEx(SunPixelShaderFile, &SunPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> SunPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(SunPixelShaderFile, SunPixelShaderByteCodeData, (DWORD)SunPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(SunPixelShaderFile);

		VkShaderModule SkyVertexShaderModule, SkyPixelShaderModule, SunVertexShaderModule, SunPixelShaderModule;

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = SkyVertexShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = SkyVertexShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &SkyVertexShaderModule));

		ShaderModuleCreateInfo.codeSize = SkyPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = SkyPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &SkyPixelShaderModule));

		ShaderModuleCreateInfo.codeSize = SunVertexShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = SunVertexShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &SunVertexShaderModule));

		ShaderModuleCreateInfo.codeSize = SunPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = SunPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &SunPixelShaderModule));

		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState;
		ZeroMemory(&PipelineColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
		PipelineColorBlendAttachmentState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
		
		VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo;
		ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
		PipelineColorBlendStateCreateInfo.attachmentCount = 1;
		PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
		PipelineColorBlendStateCreateInfo.pNext = nullptr;
		PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo;
		ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
		PipelineDepthStencilStateCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_GREATER;
		PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
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
		PipelineShaderStageCreateInfos[0].module = SkyVertexShaderModule;
		PipelineShaderStageCreateInfos[0].pName = "VS";
		PipelineShaderStageCreateInfos[0].pNext = nullptr;
		PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		PipelineShaderStageCreateInfos[1].flags = 0;
		PipelineShaderStageCreateInfos[1].module = SkyPixelShaderModule;
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
		GraphicsPipelineCreateInfo.layout = SkyAndSunPipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = SkyAndSunRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &SkyPipeline));

		ZeroMemory(&PipelineColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
		PipelineColorBlendAttachmentState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
		PipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
		PipelineColorBlendAttachmentState.alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
		PipelineColorBlendAttachmentState.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
		PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
		PipelineColorBlendAttachmentState.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
		PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
		PipelineColorBlendAttachmentState.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
		PipelineColorBlendStateCreateInfo.attachmentCount = 1;
		PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
		PipelineColorBlendStateCreateInfo.pNext = nullptr;
		PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
		PipelineDepthStencilStateCreateInfo.depthCompareOp = VkCompareOp::VK_COMPARE_OP_GREATER;
		PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		PipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
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
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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
		PipelineShaderStageCreateInfos[0].module = SunVertexShaderModule;
		PipelineShaderStageCreateInfos[0].pName = "VS";
		PipelineShaderStageCreateInfos[0].pNext = nullptr;
		PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		PipelineShaderStageCreateInfos[1].flags = 0;
		PipelineShaderStageCreateInfos[1].module = SunPixelShaderModule;
		PipelineShaderStageCreateInfos[1].pName = "PS";
		PipelineShaderStageCreateInfos[1].pNext = nullptr;
		PipelineShaderStageCreateInfos[1].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		PipelineShaderStageCreateInfos[1].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

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

		VertexInputBindingDescription.binding = 0;
		VertexInputBindingDescription.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
		VertexInputBindingDescription.stride = 56;

		PipelineVertexInputStateCreateInfo.flags = 0;
		PipelineVertexInputStateCreateInfo.pNext = nullptr;
		PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttributeDescriptions;
		PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &VertexInputBindingDescription;
		PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 5;
		PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;

		PipelineViewportStateCreateInfo.flags = 0;
		PipelineViewportStateCreateInfo.pNext = nullptr;
		PipelineViewportStateCreateInfo.pScissors = nullptr;
		PipelineViewportStateCreateInfo.pViewports = nullptr;
		PipelineViewportStateCreateInfo.scissorCount = 1;
		PipelineViewportStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		PipelineViewportStateCreateInfo.viewportCount = 1;

		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;
		GraphicsPipelineCreateInfo.flags = 0;
		GraphicsPipelineCreateInfo.layout = SkyAndSunPipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = SkyAndSunRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &SunPipeline));

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;
		DescriptorSetLayoutBinding.binding = 0;
		DescriptorSetLayoutBinding.descriptorCount = 1;
		DescriptorSetLayoutBinding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBinding.pImmutableSamplers = nullptr;
		DescriptorSetLayoutBinding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;		

		DescriptorSetLayoutCreateInfo.bindingCount = 1;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = &DescriptorSetLayoutBinding;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &FogSetLayout));

		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &FogSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &FogPipelineLayout));

		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &FogSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &FogSets[0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &FogSets[1]));

		VkShaderModule FogShaderModule;

		HANDLE FogPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/Fog.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER FogPixelShaderByteCodeLength;
		Result = GetFileSizeEx(FogPixelShaderFile, &FogPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> FogPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FogPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(FogPixelShaderFile, FogPixelShaderByteCodeData, (DWORD)FogPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(FogPixelShaderFile);

		ShaderModuleCreateInfo.codeSize = FogPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = FogPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &FogShaderModule));

		ZeroMemory(&PipelineColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
		PipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
		PipelineColorBlendAttachmentState.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
		PipelineColorBlendAttachmentState.alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
		PipelineColorBlendAttachmentState.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
		PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
		PipelineColorBlendAttachmentState.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
		PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
		PipelineColorBlendAttachmentState.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		ZeroMemory(&PipelineColorBlendStateCreateInfo, sizeof(VkPipelineColorBlendStateCreateInfo));
		PipelineColorBlendStateCreateInfo.attachmentCount = 1;
		PipelineColorBlendStateCreateInfo.flags = 0;
		PipelineColorBlendStateCreateInfo.logicOp = (VkLogicOp)0;
		PipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		PipelineColorBlendStateCreateInfo.pAttachments = &PipelineColorBlendAttachmentState;
		PipelineColorBlendStateCreateInfo.pNext = nullptr;
		PipelineColorBlendStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		ZeroMemory(&PipelineDepthStencilStateCreateInfo, sizeof(VkPipelineDepthStencilStateCreateInfo));
		PipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
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
		PipelineInputAssemblyStateCreateInfo.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

		ZeroMemory(&PipelineMultisampleStateCreateInfo, sizeof(VkPipelineMultisampleStateCreateInfo));
		PipelineMultisampleStateCreateInfo.pNext = nullptr;
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		PipelineMultisampleStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		ZeroMemory(&PipelineRasterizationStateCreateInfo, sizeof(PipelineRasterizationStateCreateInfo));
		PipelineRasterizationStateCreateInfo.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
		PipelineRasterizationStateCreateInfo.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
		PipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
		PipelineRasterizationStateCreateInfo.pNext = nullptr;
		PipelineRasterizationStateCreateInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
		PipelineRasterizationStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		PipelineShaderStageCreateInfos[0].flags = 0;
		PipelineShaderStageCreateInfos[0].module = FullScreenQuadShaderModule;
		PipelineShaderStageCreateInfos[0].pName = "VS";
		PipelineShaderStageCreateInfos[0].pNext = nullptr;
		PipelineShaderStageCreateInfos[0].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[0].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		PipelineShaderStageCreateInfos[0].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		PipelineShaderStageCreateInfos[1].flags = 0;
		PipelineShaderStageCreateInfos[1].module = FogShaderModule;
		PipelineShaderStageCreateInfos[1].pName = "PS";
		PipelineShaderStageCreateInfos[1].pNext = nullptr;
		PipelineShaderStageCreateInfos[1].pSpecializationInfo = nullptr;
		PipelineShaderStageCreateInfos[1].stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		PipelineShaderStageCreateInfos[1].sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		PipelineVertexInputStateCreateInfo.flags = 0;
		PipelineVertexInputStateCreateInfo.pNext = nullptr;
		PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
		PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;

		PipelineViewportStateCreateInfo.flags = 0;
		PipelineViewportStateCreateInfo.pNext = nullptr;
		PipelineViewportStateCreateInfo.pScissors = nullptr;
		PipelineViewportStateCreateInfo.pViewports = nullptr;
		PipelineViewportStateCreateInfo.scissorCount = 1;
		PipelineViewportStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		PipelineViewportStateCreateInfo.viewportCount = 1;

		GraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		GraphicsPipelineCreateInfo.basePipelineIndex = -1;
		GraphicsPipelineCreateInfo.flags = 0;
		GraphicsPipelineCreateInfo.layout = FogPipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = DeferredLightingRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &FogPipeline));

	}

	// ===============================================================================================================

	{
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

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &ResolvedHDRSceneColorTexture));

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, ResolvedHDRSceneColorTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &ResolvedHDRSceneColorTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, ResolvedHDRSceneColorTexture, ResolvedHDRSceneColorTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		ImageViewCreateInfo.image = ResolvedHDRSceneColorTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &ResolvedHDRSceneColorTextureView));

		VkAttachmentDescription AttachmentDescriptions[2];
		AttachmentDescriptions[0].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].flags = 0;
		AttachmentDescriptions[0].format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		AttachmentDescriptions[0].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[0].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[0].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[0].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[0].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[1].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].flags = 0;
		AttachmentDescriptions[1].format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		AttachmentDescriptions[1].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[1].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescriptions[1].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[1].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[1].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference AttachmentReferences[2];
		AttachmentReferences[0].attachment = 0;
		AttachmentReferences[0].layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentReferences[1].attachment = 1;
		AttachmentReferences[1].layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription SubpassDescription;
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.flags = 0;
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = &AttachmentReferences[0];
		SubpassDescription.pDepthStencilAttachment = nullptr;
		SubpassDescription.pInputAttachments = nullptr;
		SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.pPreserveAttachments = nullptr;
		SubpassDescription.preserveAttachmentCount = 0;
		SubpassDescription.pResolveAttachments = &AttachmentReferences[1];

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

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &HDRSceneColorResolveRenderPass));

		VkImageView Attachments[2] = { HDRSceneColorTextureView, ResolvedHDRSceneColorTextureView };

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 2;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = Attachments;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = HDRSceneColorResolveRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &HDRSceneColorResolveFrameBuffer));
	}

	// ===============================================================================================================
	
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

		for (int i = 0; i < 4; i++)
		{
			ImageCreateInfo.extent.height = Heights[i];
			ImageCreateInfo.extent.width = Widths[i];

			SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &SceneLuminanceTextures[i]));

			VkMemoryRequirements MemoryRequirements;

			vkGetImageMemoryRequirements(Device, SceneLuminanceTextures[i], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &SceneLuminanceTexturesMemoryHeaps[i]));

			SAFE_VK(vkBindImageMemory(Device, SceneLuminanceTextures[i], SceneLuminanceTexturesMemoryHeaps[i], 0));
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

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &AverageLuminanceTexture));

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, AverageLuminanceTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &AverageLuminanceTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, AverageLuminanceTexture, AverageLuminanceTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R32_SFLOAT;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		ImageViewCreateInfo.image = SceneLuminanceTextures[0];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SceneLuminanceTexturesViews[0]));
		ImageViewCreateInfo.image = SceneLuminanceTextures[1];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SceneLuminanceTexturesViews[1]));
		ImageViewCreateInfo.image = SceneLuminanceTextures[2];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SceneLuminanceTexturesViews[2]));
		ImageViewCreateInfo.image = SceneLuminanceTextures[3];
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &SceneLuminanceTexturesViews[3]));


		ImageViewCreateInfo.image = AverageLuminanceTexture;
		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &AverageLuminanceTextureView));

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[2];
		DescriptorSetLayoutBindings[0].binding = 0;
		DescriptorSetLayoutBindings[0].descriptorCount = 1;
		DescriptorSetLayoutBindings[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		DescriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[0].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
		DescriptorSetLayoutBindings[1].binding = 1;
		DescriptorSetLayoutBindings[1].descriptorCount = 1;
		DescriptorSetLayoutBindings[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		DescriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[1].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
		DescriptorSetLayoutCreateInfo.bindingCount = 2;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &LuminancePassSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &LuminancePassSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &LuminancePassPipelineLayout));

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &LuminancePassSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[3][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[4][0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[3][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &LuminancePassSets[4][1]));

		HANDLE LuminanceCalcComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceCalc.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER LuminanceCalcComputeShaderByteCodeLength;
		Result = GetFileSizeEx(LuminanceCalcComputeShaderFile, &LuminanceCalcComputeShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceCalcComputeShaderByteCodeLength.QuadPart);
		Result = ReadFile(LuminanceCalcComputeShaderFile, LuminanceCalcComputeShaderByteCodeData, (DWORD)LuminanceCalcComputeShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(LuminanceCalcComputeShaderFile);

		HANDLE LuminanceSumComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceSum.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER LuminanceSumComputeShaderByteCodeLength;
		Result = GetFileSizeEx(LuminanceSumComputeShaderFile, &LuminanceSumComputeShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceSumComputeShaderByteCodeLength.QuadPart);
		Result = ReadFile(LuminanceSumComputeShaderFile, LuminanceSumComputeShaderByteCodeData, (DWORD)LuminanceSumComputeShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(LuminanceSumComputeShaderFile);

		HANDLE LuminanceAvgComputeShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/LuminanceAvg.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER LuminanceAvgComputeShaderByteCodeLength;
		Result = GetFileSizeEx(LuminanceAvgComputeShaderFile, &LuminanceAvgComputeShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceAvgComputeShaderByteCodeLength.QuadPart);
		Result = ReadFile(LuminanceAvgComputeShaderFile, LuminanceAvgComputeShaderByteCodeData, (DWORD)LuminanceAvgComputeShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(LuminanceAvgComputeShaderFile);

		VkShaderModule LuminanceCalcComputeShaderModule, LuminanceSumComputeShaderModule, LuminanceAvgComputeShaderModule;

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = LuminanceCalcComputeShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = LuminanceCalcComputeShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &LuminanceCalcComputeShaderModule));

		ShaderModuleCreateInfo.codeSize = LuminanceSumComputeShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = LuminanceSumComputeShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &LuminanceSumComputeShaderModule));

		ShaderModuleCreateInfo.codeSize = LuminanceAvgComputeShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = LuminanceAvgComputeShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &LuminanceAvgComputeShaderModule));

		VkComputePipelineCreateInfo ComputePipelineCreateInfo;
		ComputePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		ComputePipelineCreateInfo.basePipelineIndex = -1;
		ComputePipelineCreateInfo.flags = 0;
		ComputePipelineCreateInfo.layout = LuminancePassPipelineLayout;
		ComputePipelineCreateInfo.pNext = nullptr;
		ComputePipelineCreateInfo.stage.flags = 0;
		ComputePipelineCreateInfo.stage.pName = "CS";
		ComputePipelineCreateInfo.stage.pNext = nullptr;
		ComputePipelineCreateInfo.stage.pSpecializationInfo = nullptr;
		ComputePipelineCreateInfo.stage.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
		ComputePipelineCreateInfo.stage.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ComputePipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

		ComputePipelineCreateInfo.stage.module = LuminanceCalcComputeShaderModule;
		SAFE_VK(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &ComputePipelineCreateInfo, nullptr, &LuminanceCalcPipeline));
		
		ComputePipelineCreateInfo.stage.module = LuminanceSumComputeShaderModule;
		SAFE_VK(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &ComputePipelineCreateInfo, nullptr, &LuminanceSumPipeline));
		
		ComputePipelineCreateInfo.stage.module = LuminanceAvgComputeShaderModule;
		SAFE_VK(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &ComputePipelineCreateInfo, nullptr, &LuminanceAvgPipeline));
	}
	
	// ===============================================================================================================
	
	{
		VkAttachmentDescription AttachmentDescription;
		AttachmentDescription.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.flags = 0;
		AttachmentDescription.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
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

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &BloomRenderPass));

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = BloomRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		
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

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		for (int i = 0; i < 7; i++)
		{
			ImageCreateInfo.extent.height = ResolutionHeight >> i;
			ImageCreateInfo.extent.width = ResolutionWidth >> i;

			SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[0][i]));
			SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[1][i]));
			SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[2][i]));

			VkMemoryRequirements MemoryRequirements;

			vkGetImageMemoryRequirements(Device, BloomTextures[0][i], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BloomTexturesMemoryHeaps[0][i]));

			SAFE_VK(vkBindImageMemory(Device, BloomTextures[0][i], BloomTexturesMemoryHeaps[0][i], 0));

			vkGetImageMemoryRequirements(Device, BloomTextures[1][i], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BloomTexturesMemoryHeaps[1][i]));

			SAFE_VK(vkBindImageMemory(Device, BloomTextures[1][i], BloomTexturesMemoryHeaps[1][i], 0));

			vkGetImageMemoryRequirements(Device, BloomTextures[2][i], &MemoryRequirements);

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

			SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BloomTexturesMemoryHeaps[2][i]));

			SAFE_VK(vkBindImageMemory(Device, BloomTextures[2][i], BloomTexturesMemoryHeaps[2][i], 0));

			ImageViewCreateInfo.image = BloomTextures[0][i];
			SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &BloomTexturesViews[0][i]));
			ImageViewCreateInfo.image = BloomTextures[1][i];
			SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &BloomTexturesViews[1][i]));
			ImageViewCreateInfo.image = BloomTextures[2][i];
			SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &BloomTexturesViews[2][i]));

			FramebufferCreateInfo.height = ResolutionHeight >> i;
			FramebufferCreateInfo.width = ResolutionWidth >> i;

			FramebufferCreateInfo.pAttachments = &BloomTexturesViews[0][i];
			SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &BloomTexturesFrameBuffers[0][i]));
			FramebufferCreateInfo.pAttachments = &BloomTexturesViews[1][i];
			SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &BloomTexturesFrameBuffers[1][i]));
			FramebufferCreateInfo.pAttachments = &BloomTexturesViews[2][i];
			SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &BloomTexturesFrameBuffers[2][i]));
		}

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[3];
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
		DescriptorSetLayoutBindings[2].binding = 2;
		DescriptorSetLayoutBindings[2].descriptorCount = 1;
		DescriptorSetLayoutBindings[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		DescriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;
		DescriptorSetLayoutBindings[2].stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
		DescriptorSetLayoutCreateInfo.bindingCount = 3;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &BloomPassSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &BloomPassSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &BloomPassPipelineLayout));

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &BloomPassSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets1[0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets1[1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets1[2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[0][0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[0][1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[0][2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[1][0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[1][1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[1][2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[2][0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[2][1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[2][2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[3][0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[3][1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[3][2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[4][0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[4][1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[4][2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[5][0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[5][1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[5][2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[0][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[1][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[2][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[3][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[4][0]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[5][0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets1[0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets1[1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets1[2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[0][0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[0][1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[0][2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[1][0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[1][1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[1][2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[2][0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[2][1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[2][2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[3][0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[3][1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[3][2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[4][0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[4][1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[4][2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[5][0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[5][1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets2[5][2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[0][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[1][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[2][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[3][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[4][1]));
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &BloomPassSets3[5][1]));

		HANDLE BrightPassPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/BrightPass.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER BrightPassPixelShaderByteCodeLength;
		Result = GetFileSizeEx(BrightPassPixelShaderFile, &BrightPassPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> BrightPassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(BrightPassPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(BrightPassPixelShaderFile, BrightPassPixelShaderByteCodeData, (DWORD)BrightPassPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(BrightPassPixelShaderFile);

		HANDLE ImageResamplePixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/ImageResample.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER ImageResamplePixelShaderByteCodeLength;
		Result = GetFileSizeEx(ImageResamplePixelShaderFile, &ImageResamplePixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> ImageResamplePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ImageResamplePixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(ImageResamplePixelShaderFile, ImageResamplePixelShaderByteCodeData, (DWORD)ImageResamplePixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(ImageResamplePixelShaderFile);

		HANDLE HorizontalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HorizontalBlur.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER HorizontalBlurPixelShaderByteCodeLength;
		Result = GetFileSizeEx(HorizontalBlurPixelShaderFile, &HorizontalBlurPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> HorizontalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HorizontalBlurPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(HorizontalBlurPixelShaderFile, HorizontalBlurPixelShaderByteCodeData, (DWORD)HorizontalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(HorizontalBlurPixelShaderFile);

		HANDLE VerticalBlurPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/VerticalBlur.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER VerticalBlurPixelShaderByteCodeLength;
		Result = GetFileSizeEx(VerticalBlurPixelShaderFile, &VerticalBlurPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> VerticalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(VerticalBlurPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(VerticalBlurPixelShaderFile, VerticalBlurPixelShaderByteCodeData, (DWORD)VerticalBlurPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(VerticalBlurPixelShaderFile);

		VkShaderModule BrightPassPixelShaderModule, ImageResamplePixelShaderModule, HorizontalBlurPixelShaderModule, VerticalBlurPixelShaderModule;

		ShaderModuleCreateInfo.codeSize = BrightPassPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = BrightPassPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &BrightPassPixelShaderModule));

		ShaderModuleCreateInfo.codeSize = ImageResamplePixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = ImageResamplePixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ImageResamplePixelShaderModule));

		ShaderModuleCreateInfo.codeSize = HorizontalBlurPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = HorizontalBlurPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &HorizontalBlurPixelShaderModule));

		ShaderModuleCreateInfo.codeSize = VerticalBlurPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = VerticalBlurPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &VerticalBlurPixelShaderModule));

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
		PipelineShaderStageCreateInfos[1].module = BrightPassPixelShaderModule;
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
		GraphicsPipelineCreateInfo.layout = BloomPassPipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = BloomRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &BrightPassPipeline));

		PipelineShaderStageCreateInfos[1].module = HorizontalBlurPixelShaderModule;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &HorizontalBlurPipeline));

		PipelineShaderStageCreateInfos[1].module = VerticalBlurPixelShaderModule;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &VerticalBlurPipeline));

		PipelineShaderStageCreateInfos[1].module = ImageResamplePixelShaderModule;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &DownSamplePipeline));

		PipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
		PipelineColorBlendAttachmentState.alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
		PipelineColorBlendAttachmentState.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD;
		PipelineColorBlendAttachmentState.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
		PipelineColorBlendAttachmentState.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;
		PipelineColorBlendAttachmentState.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO;
		PipelineColorBlendAttachmentState.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &UpSampleWithAddBlendPipeline));
	}
	 
	// ===============================================================================================================
	
	{
		VkImageCreateInfo ImageCreateInfo;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.extent.height = ResolutionHeight;
		ImageCreateInfo.extent.width = ResolutionWidth;
		ImageCreateInfo.flags = 0;
		ImageCreateInfo.format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
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
		ImageCreateInfo.usage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &ToneMappedImageTexture));

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, ToneMappedImageTexture, &MemoryRequirements);

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

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &ToneMappedImageTextureMemoryHeap));

		SAFE_VK(vkBindImageMemory(Device, ToneMappedImageTexture, ToneMappedImageTextureMemoryHeap, 0));

		VkImageViewCreateInfo ImageViewCreateInfo;
		ImageViewCreateInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
		ImageViewCreateInfo.flags = 0;
		ImageViewCreateInfo.format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
		ImageViewCreateInfo.image = ToneMappedImageTexture;
		ImageViewCreateInfo.pNext = nullptr;
		ImageViewCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ImageViewCreateInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;

		SAFE_VK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &ToneMappedImageTextureView));

		VkAttachmentDescription AttachmentDescription;
		AttachmentDescription.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.flags = 0;
		AttachmentDescription.format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
		AttachmentDescription.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescription.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescription.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &HDRToneMappingRenderPass));

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = &ToneMappedImageTextureView;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = HDRToneMappingRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &ToneMappedImageFrameBuffer));

		VkDescriptorSetLayoutBinding DescriptorSetLayoutBindings[2];
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

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo;
		DescriptorSetLayoutCreateInfo.bindingCount = 2;
		DescriptorSetLayoutCreateInfo.flags = 0;
		DescriptorSetLayoutCreateInfo.pBindings = DescriptorSetLayoutBindings;
		DescriptorSetLayoutCreateInfo.pNext = nullptr;
		DescriptorSetLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutCreateInfo, nullptr, &HDRToneMappingSetLayout));

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
		PipelineLayoutCreateInfo.flags = 0;
		PipelineLayoutCreateInfo.pNext = nullptr;
		PipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		PipelineLayoutCreateInfo.pSetLayouts = &HDRToneMappingSetLayout;
		PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
		PipelineLayoutCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		SAFE_VK(vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &HDRToneMappingPipelineLayout));

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
		DescriptorSetAllocateInfo.descriptorSetCount = 1;
		DescriptorSetAllocateInfo.pNext = nullptr;
		DescriptorSetAllocateInfo.pSetLayouts = &HDRToneMappingSetLayout;
		DescriptorSetAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[0];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &HDRToneMappingSets[0]));
		DescriptorSetAllocateInfo.descriptorPool = DescriptorPools[1];
		SAFE_VK(vkAllocateDescriptorSets(Device, &DescriptorSetAllocateInfo, &HDRToneMappingSets[1]));

		HANDLE HDRToneMappingPixelShaderFile = CreateFile((const wchar_t*)u"GameContent/Shaders/HDRToneMapping.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		LARGE_INTEGER HDRToneMappingPixelShaderByteCodeLength;
		Result = GetFileSizeEx(HDRToneMappingPixelShaderFile, &HDRToneMappingPixelShaderByteCodeLength);
		ScopedMemoryBlockArray<BYTE> HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HDRToneMappingPixelShaderByteCodeLength.QuadPart);
		Result = ReadFile(HDRToneMappingPixelShaderFile, HDRToneMappingPixelShaderByteCodeData, (DWORD)HDRToneMappingPixelShaderByteCodeLength.QuadPart, NULL, NULL);
		Result = CloseHandle(HDRToneMappingPixelShaderFile);

		VkShaderModule HDRToneMappingPixelShaderModule;

		ShaderModuleCreateInfo.codeSize = HDRToneMappingPixelShaderByteCodeLength.QuadPart;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = HDRToneMappingPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &HDRToneMappingPixelShaderModule));

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
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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
		PipelineShaderStageCreateInfos[1].module = HDRToneMappingPixelShaderModule;
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
		GraphicsPipelineCreateInfo.layout = HDRToneMappingPipelineLayout;
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
		GraphicsPipelineCreateInfo.renderPass = HDRToneMappingRenderPass;
		GraphicsPipelineCreateInfo.stageCount = 2;
		GraphicsPipelineCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		GraphicsPipelineCreateInfo.subpass = 0;

		SAFE_VK(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &GraphicsPipelineCreateInfo, nullptr, &HDRToneMappingPipeline));
	}
	
	// ===============================================================================================================
	
	{
		VkAttachmentDescription AttachmentDescriptions[2];
		AttachmentDescriptions[0].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].flags = 0;
		AttachmentDescriptions[0].format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
		AttachmentDescriptions[0].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[0].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[0].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
		AttachmentDescriptions[0].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[0].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[0].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
		AttachmentDescriptions[1].finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].flags = 0;
		AttachmentDescriptions[1].format = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
		AttachmentDescriptions[1].initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentDescriptions[1].loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
		AttachmentDescriptions[1].samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		AttachmentDescriptions[1].stencilLoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentDescriptions[1].stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentDescriptions[1].storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentReference AttachmentReferences[2];
		AttachmentReferences[0].attachment = 0;
		AttachmentReferences[0].layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		AttachmentReferences[1].attachment = 1;
		AttachmentReferences[1].layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription SubpassDescription;
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.flags = 0;
		SubpassDescription.inputAttachmentCount = 0;
		SubpassDescription.pColorAttachments = &AttachmentReferences[0];
		SubpassDescription.pDepthStencilAttachment = nullptr;
		SubpassDescription.pInputAttachments = nullptr;
		SubpassDescription.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.pPreserveAttachments = nullptr;
		SubpassDescription.preserveAttachmentCount = 0;
		SubpassDescription.pResolveAttachments = &AttachmentReferences[1];

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

		SAFE_VK(vkCreateRenderPass(Device, &RenderPassCreateInfo, nullptr, &BackBufferResolveRenderPass));

		BackBufferFrameBuffers = new VkFramebuffer[SwapChainImagesCount];

		VkImageView Attachments[2];

		Attachments[0] = ToneMappedImageTextureView;

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 2;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.height = ResolutionHeight;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pAttachments = Attachments;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = BackBufferResolveRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FramebufferCreateInfo.width = ResolutionWidth;

		for (uint32_t i = 0; i < SwapChainImagesCount; i++)
		{
			Attachments[1] = BackBufferTexturesViews[i];

			SAFE_VK(vkCreateFramebuffer(Device, &FramebufferCreateInfo, nullptr, &BackBufferFrameBuffers[i]));
		}
	}
	
	// ===============================================================================================================
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

	{
		VkImageMemoryBarrier ImageMemoryBarriers[5];

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = GBufferTextures[0];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = GBufferTextures[1];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[1].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[1].pNext = nullptr;
		ImageMemoryBarriers[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[1].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[1].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[1].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[1].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[1].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[2].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].image = DepthBufferTexture;
		ImageMemoryBarriers[2].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[2].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[2].pNext = nullptr;
		ImageMemoryBarriers[2].srcAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[2].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		ImageMemoryBarriers[2].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[2].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[2].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[2].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[3].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[3].image = ShadowMaskTexture;
		ImageMemoryBarriers[3].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[3].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[3].pNext = nullptr;
		ImageMemoryBarriers[3].srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[3].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[3].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[3].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[3].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[3].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[3].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[4].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[4].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[4].image = HDRSceneColorTexture;
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 5, ImageMemoryBarriers);

		void *ConstantBufferData;

		SAFE_VK(vkMapMemory(Device, CPUDeferredLightingConstantBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &ConstantBufferData));

		XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

		DeferredLightingConstantBuffer& ConstantBuffer = *((DeferredLightingConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.InvViewProjMatrix = InvViewProjMatrix;
		ConstantBuffer.CameraWorldPosition = CameraLocation;

		vkUnmapMemory(Device, CPUDeferredLightingConstantBuffersMemoryHeaps[CurrentFrameIndex]);

		void *DynamicBufferData;

		SAFE_VK(vkMapMemory(Device, CPULightClustersBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &DynamicBufferData));

		memcpy(DynamicBufferData, clusterizationSubSystem.GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

		vkUnmapMemory(Device, CPULightClustersBuffersMemoryHeaps[CurrentFrameIndex]);

		SAFE_VK(vkMapMemory(Device, CPULightIndicesBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &DynamicBufferData));

		memcpy(DynamicBufferData, clusterizationSubSystem.GetLightIndicesData(), clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t));

		vkUnmapMemory(Device, CPULightIndicesBuffersMemoryHeaps[CurrentFrameIndex]);

		SAFE_VK(vkMapMemory(Device, CPUPointLightsBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &DynamicBufferData));

		memcpy(DynamicBufferData, PointLights.data(), PointLights.size() * sizeof(PointLight));

		vkUnmapMemory(Device, CPUPointLightsBuffersMemoryHeaps[CurrentFrameIndex]);

		VkBufferMemoryBarrier BufferMemoryBarriers[4];

		BufferMemoryBarriers[0].buffer = CPUDeferredLightingConstantBuffers[CurrentFrameIndex];
		BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].offset = 0;
		BufferMemoryBarriers[0].pNext = nullptr;
		BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
		BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[1].buffer = CPULightClustersBuffers[CurrentFrameIndex];
		BufferMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].offset = 0;
		BufferMemoryBarriers[1].pNext = nullptr;
		BufferMemoryBarriers[1].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
		BufferMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[2].buffer = CPULightIndicesBuffers[CurrentFrameIndex];
		BufferMemoryBarriers[2].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[2].offset = 0;
		BufferMemoryBarriers[2].pNext = nullptr;
		BufferMemoryBarriers[2].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[2].srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
		BufferMemoryBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[2].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[3].buffer = CPUPointLightsBuffers[CurrentFrameIndex];
		BufferMemoryBarriers[3].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[3].offset = 0;
		BufferMemoryBarriers[3].pNext = nullptr;
		BufferMemoryBarriers[3].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[3].srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
		BufferMemoryBarriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[3].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 4, BufferMemoryBarriers, 0, nullptr);

		VkBufferCopy BufferCopy;
		BufferCopy.dstOffset = 0;
		BufferCopy.srcOffset = 0;

		BufferCopy.size = 256;
		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPUDeferredLightingConstantBuffers[CurrentFrameIndex], GPUDeferredLightingConstantBuffer, 1, &BufferCopy);
		BufferCopy.size = ClusterizationSubSystem::CLUSTERS_COUNT_X * ClusterizationSubSystem::CLUSTERS_COUNT_Y * ClusterizationSubSystem::CLUSTERS_COUNT_Z * sizeof(LightCluster);
		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPULightClustersBuffers[CurrentFrameIndex], GPULightClustersBuffer, 1, &BufferCopy);
		BufferCopy.size = clusterizationSubSystem.GetTotalIndexCount() * sizeof(uint16_t);
		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPULightIndicesBuffers[CurrentFrameIndex], GPULightIndicesBuffer, 1, &BufferCopy);
		BufferCopy.size = PointLights.size() * sizeof(PointLight);
		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPUPointLightsBuffers[CurrentFrameIndex], GPUPointLightsBuffer, 1, &BufferCopy);

		BufferMemoryBarriers[0].buffer = GPUDeferredLightingConstantBuffer;
		BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].offset = 0;
		BufferMemoryBarriers[0].pNext = nullptr;
		BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[1].buffer = GPULightClustersBuffer;
		BufferMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		BufferMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].offset = 0;
		BufferMemoryBarriers[1].pNext = nullptr;
		BufferMemoryBarriers[1].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[2].buffer = GPULightIndicesBuffer;
		BufferMemoryBarriers[2].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		BufferMemoryBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[2].offset = 0;
		BufferMemoryBarriers[2].pNext = nullptr;
		BufferMemoryBarriers[2].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[2].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[2].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[3].buffer = GPUPointLightsBuffer;
		BufferMemoryBarriers[3].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		BufferMemoryBarriers[3].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[3].offset = 0;
		BufferMemoryBarriers[3].pNext = nullptr;
		BufferMemoryBarriers[3].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[3].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[3].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[3].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 4, BufferMemoryBarriers, 0, nullptr);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = HDRSceneColorFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = DeferredLightingRenderPass;
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

		VkDescriptorBufferInfo DescriptorBufferInfos[2];
		DescriptorBufferInfos[0].buffer = GPUDeferredLightingConstantBuffer;
		DescriptorBufferInfos[0].offset = 0;
		DescriptorBufferInfos[0].range = 256;
		DescriptorBufferInfos[1].buffer = GPUPointLightsBuffer;
		DescriptorBufferInfos[1].offset = 0;
		DescriptorBufferInfos[1].range = 10000 * 2 * 4 * sizeof(float);

		VkDescriptorImageInfo DescriptorImageInfos[4];
		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = GBufferTexturesViews[0];
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[1].imageView = GBufferTexturesViews[1];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[2].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[2].imageView = DepthBufferTextureDepthReadView;
		DescriptorImageInfos[2].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[3].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[3].imageView = ShadowMaskTextureView;
		DescriptorImageInfos[3].sampler = VK_NULL_HANDLE;

		VkBufferView BufferViews[2] = { LightClustersBufferView, LightIndicesBufferView };

		VkWriteDescriptorSet WriteDescriptorSets[4];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = DeferredLightingSets[CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = &DescriptorBufferInfos[0];
		WriteDescriptorSets[0].pImageInfo = nullptr;
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 4;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = DeferredLightingSets[CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[2].descriptorCount = 2;
		WriteDescriptorSets[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		WriteDescriptorSets[2].dstArrayElement = 0;
		WriteDescriptorSets[2].dstBinding = 5;
		WriteDescriptorSets[2].dstSet = DeferredLightingSets[CurrentFrameIndex];
		WriteDescriptorSets[2].pBufferInfo = nullptr;
		WriteDescriptorSets[2].pImageInfo = nullptr;
		WriteDescriptorSets[2].pNext = nullptr;
		WriteDescriptorSets[2].pTexelBufferView = BufferViews;
		WriteDescriptorSets[2].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[3].descriptorCount = 1;
		WriteDescriptorSets[3].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		WriteDescriptorSets[3].dstArrayElement = 0;
		WriteDescriptorSets[3].dstBinding = 7;
		WriteDescriptorSets[3].dstSet = DeferredLightingSets[CurrentFrameIndex];
		WriteDescriptorSets[3].pBufferInfo = &DescriptorBufferInfos[1];
		WriteDescriptorSets[3].pImageInfo = nullptr;
		WriteDescriptorSets[3].pNext = nullptr;
		WriteDescriptorSets[3].pTexelBufferView = nullptr;
		WriteDescriptorSets[3].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 4, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredLightingPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, DeferredLightingPipelineLayout, 0, 1, &DeferredLightingSets[CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
	}

	// ===============================================================================================================	

	{
		XMMATRIX SkyWorldMatrix = XMMatrixScaling(900.0f, 900.0f, 900.0f) * XMMatrixTranslation(CameraLocation.x, CameraLocation.y, CameraLocation.z);
		XMMATRIX SkyWVPMatrix = SkyWorldMatrix * ViewProjMatrix;

		void *ConstantBufferData;

		SAFE_VK(vkMapMemory(Device, CPUSkyConstantBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &ConstantBufferData));

		SkyConstantBuffer& skyConstantBuffer = *((SkyConstantBuffer*)((BYTE*)ConstantBufferData));

		skyConstantBuffer.WVPMatrix = SkyWVPMatrix;

		vkUnmapMemory(Device, CPUSkyConstantBuffersMemoryHeaps[CurrentFrameIndex]);

		XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);

		SAFE_VK(vkMapMemory(Device, CPUSunConstantBuffersMemoryHeaps[CurrentFrameIndex], 0, VK_WHOLE_SIZE, 0, &ConstantBufferData));

		SunConstantBuffer& sunConstantBuffer = *((SunConstantBuffer*)((BYTE*)ConstantBufferData));

		sunConstantBuffer.ViewMatrix = ViewMatrix;
		sunConstantBuffer.ProjMatrix = ProjMatrix;
		sunConstantBuffer.SunPosition = SunPosition;

		vkUnmapMemory(Device, CPUSunConstantBuffersMemoryHeaps[CurrentFrameIndex]);

		VkBufferMemoryBarrier BufferMemoryBarriers[2];
		BufferMemoryBarriers[0].buffer = CPUSkyConstantBuffers[CurrentFrameIndex];
		BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].offset = 0;
		BufferMemoryBarriers[0].pNext = nullptr;
		BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
		BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[1].buffer = CPUSunConstantBuffers[CurrentFrameIndex];
		BufferMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_READ_BIT;
		BufferMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].offset = 0;
		BufferMemoryBarriers[1].pNext = nullptr;
		BufferMemoryBarriers[1].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_HOST_WRITE_BIT;
		BufferMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		
		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 2, BufferMemoryBarriers, 0, nullptr);

		VkBufferCopy BufferCopy;
		BufferCopy.dstOffset = 0;
		BufferCopy.size = 256;
		BufferCopy.srcOffset = 0;

		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPUSkyConstantBuffers[CurrentFrameIndex], GPUSkyConstantBuffer, 1, &BufferCopy);
		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPUSunConstantBuffers[CurrentFrameIndex], GPUSunConstantBuffer, 1, &BufferCopy);

		BufferMemoryBarriers[0].buffer = GPUSkyConstantBuffer;
		BufferMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		BufferMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].offset = 0;
		BufferMemoryBarriers[0].pNext = nullptr;
		BufferMemoryBarriers[0].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		BufferMemoryBarriers[1].buffer = GPUSunConstantBuffer;
		BufferMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		BufferMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].offset = 0;
		BufferMemoryBarriers[1].pNext = nullptr;
		BufferMemoryBarriers[1].size = VK_WHOLE_SIZE;
		BufferMemoryBarriers[1].srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
		BufferMemoryBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		BufferMemoryBarriers[1].sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 2, BufferMemoryBarriers, 0, nullptr);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = HDRSceneColorFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = DeferredLightingRenderPass;
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
		DescriptorImageInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfo.imageView = DepthBufferTextureDepthReadView;
		DescriptorImageInfo.sampler = VK_NULL_HANDLE;

		VkWriteDescriptorSet WriteDescriptorSet;
		WriteDescriptorSet.descriptorCount = 1;
		WriteDescriptorSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstSet = FogSets[CurrentFrameIndex];
		WriteDescriptorSet.pBufferInfo = nullptr;
		WriteDescriptorSet.pImageInfo = &DescriptorImageInfo;
		WriteDescriptorSet.pNext = nullptr;
		WriteDescriptorSet.pTexelBufferView = nullptr;
		WriteDescriptorSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		
		vkUpdateDescriptorSets(Device, 1, &WriteDescriptorSet, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, FogPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, FogPipelineLayout, 0, 1, &FogSets[CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

		VkImageMemoryBarrier ImageMemoryBarrier;
		ImageMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.image = DepthBufferTexture;
		ImageMemoryBarrier.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageMemoryBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarrier.pNext = nullptr;
		ImageMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarrier.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT | VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
		ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		ImageMemoryBarrier.subresourceRange.layerCount = 1;
		ImageMemoryBarrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrier);

		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = HDRSceneColorAndDepthFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = SkyAndSunRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

		VkDescriptorBufferInfo DescriptorBufferInfo;
		DescriptorBufferInfo.buffer = GPUSkyConstantBuffer;
		DescriptorBufferInfo.offset = 0;
		DescriptorBufferInfo.range = 256;

		VkDescriptorImageInfo DescriptorImageInfos[2];
		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = SkyTextureView;
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = (VkImageLayout)0;
		DescriptorImageInfos[1].imageView = VK_NULL_HANDLE;
		DescriptorImageInfos[1].sampler = TextureSampler;

		VkWriteDescriptorSet WriteDescriptorSets[3];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = SkyAndSunSets[0][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = &DescriptorBufferInfo;
		WriteDescriptorSets[0].pImageInfo = nullptr;
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = SkyAndSunSets[0][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[2].descriptorCount = 1;
		WriteDescriptorSets[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		WriteDescriptorSets[2].dstArrayElement = 0;
		WriteDescriptorSets[2].dstBinding = 2;
		WriteDescriptorSets[2].dstSet = SkyAndSunSets[0][CurrentFrameIndex];
		WriteDescriptorSets[2].pBufferInfo = nullptr;
		WriteDescriptorSets[2].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[2].pNext = nullptr;
		WriteDescriptorSets[2].pTexelBufferView = nullptr;
		WriteDescriptorSets[2].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 3, WriteDescriptorSets, 0, nullptr);

		VkDeviceSize Offset = 0;

		vkCmdBindVertexBuffers(CommandBuffers[CurrentFrameIndex], 0, 1, &SkyVertexBuffer, &Offset);
		vkCmdBindIndexBuffer(CommandBuffers[CurrentFrameIndex], SkyIndexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, SkyPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, SkyAndSunPipelineLayout, 0, 1, &SkyAndSunSets[0][CurrentFrameIndex], 0, nullptr);

		vkCmdDrawIndexed(CommandBuffers[CurrentFrameIndex], 300 + 24 * 600 + 300, 1, 0, 0, 0);

		DescriptorBufferInfo.buffer = GPUSunConstantBuffer;
		DescriptorBufferInfo.offset = 0;
		DescriptorBufferInfo.range = 256;

		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = SunTextureView;
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = (VkImageLayout)0;
		DescriptorImageInfos[1].imageView = VK_NULL_HANDLE;
		DescriptorImageInfos[1].sampler = TextureSampler;

		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = SkyAndSunSets[1][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = &DescriptorBufferInfo;
		WriteDescriptorSets[0].pImageInfo = nullptr;
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = SkyAndSunSets[1][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[2].descriptorCount = 1;
		WriteDescriptorSets[2].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
		WriteDescriptorSets[2].dstArrayElement = 0;
		WriteDescriptorSets[2].dstBinding = 2;
		WriteDescriptorSets[2].dstSet = SkyAndSunSets[1][CurrentFrameIndex];
		WriteDescriptorSets[2].pBufferInfo = nullptr;
		WriteDescriptorSets[2].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[2].pNext = nullptr;
		WriteDescriptorSets[2].pTexelBufferView = nullptr;
		WriteDescriptorSets[2].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 3, WriteDescriptorSets, 0, nullptr);

		vkCmdBindVertexBuffers(CommandBuffers[CurrentFrameIndex], 0, 1, &SunVertexBuffer, &Offset);
		vkCmdBindIndexBuffer(CommandBuffers[CurrentFrameIndex], SunIndexBuffer, 0, VkIndexType::VK_INDEX_TYPE_UINT16);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, SunPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, SkyAndSunPipelineLayout, 0, 1, &SkyAndSunSets[1][CurrentFrameIndex], 0, nullptr);

		vkCmdDrawIndexed(CommandBuffers[CurrentFrameIndex], 6, 1, 0, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
	}

	// ===============================================================================================================	

	// ===============================================================================================================

	{
		VkImageMemoryBarrier ImageMemoryBarrier;
		ImageMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.image = ResolvedHDRSceneColorTexture;
		ImageMemoryBarrier.newLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageMemoryBarrier.oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
		ImageMemoryBarrier.pNext = nullptr;
		ImageMemoryBarrier.srcAccessMask = 0;
		ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarrier.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		ImageMemoryBarrier.subresourceRange.layerCount = 1;
		ImageMemoryBarrier.subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrier);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = HDRSceneColorResolveFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = HDRSceneColorResolveRenderPass;
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
		ImageMemoryBarriers[0].image = ResolvedHDRSceneColorTexture;
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = SceneLuminanceTextures[0];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		VkDescriptorImageInfo DescriptorImageInfos[2];
		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = ResolvedHDRSceneColorTextureView;
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		DescriptorImageInfos[1].imageView = SceneLuminanceTexturesViews[0];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

		VkWriteDescriptorSet WriteDescriptorSets[2];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = LuminancePassSets[0][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = LuminancePassSets[0][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminanceCalcPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminancePassPipelineLayout, 0, 1, &LuminancePassSets[0][CurrentFrameIndex], 0, nullptr);

		vkCmdDispatch(CommandBuffers[CurrentFrameIndex], 80, 45, 1);

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = SceneLuminanceTextures[0];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = SceneLuminanceTextures[1];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = SceneLuminanceTexturesViews[0];
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		DescriptorImageInfos[1].imageView = SceneLuminanceTexturesViews[1];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = LuminancePassSets[1][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = LuminancePassSets[1][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminanceSumPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminancePassPipelineLayout, 0, 1, &LuminancePassSets[1][CurrentFrameIndex], 0, nullptr);

		vkCmdDispatch(CommandBuffers[CurrentFrameIndex], 80, 45, 1);

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = SceneLuminanceTextures[1];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = SceneLuminanceTextures[2];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = SceneLuminanceTexturesViews[1];
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		DescriptorImageInfos[1].imageView = SceneLuminanceTexturesViews[2];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = LuminancePassSets[2][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = LuminancePassSets[2][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminanceSumPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminancePassPipelineLayout, 0, 1, &LuminancePassSets[2][CurrentFrameIndex], 0, nullptr);

		vkCmdDispatch(CommandBuffers[CurrentFrameIndex], 5, 3, 1);

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = SceneLuminanceTextures[2];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = SceneLuminanceTextures[3];
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = SceneLuminanceTexturesViews[2];
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		DescriptorImageInfos[1].imageView = SceneLuminanceTexturesViews[3];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = LuminancePassSets[3][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = LuminancePassSets[3][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminanceSumPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminancePassPipelineLayout, 0, 1, &LuminancePassSets[3][CurrentFrameIndex], 0, nullptr);

		vkCmdDispatch(CommandBuffers[CurrentFrameIndex], 1, 1, 1);

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = SceneLuminanceTextures[3];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[0].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		ImageMemoryBarriers[0].pNext = nullptr;
		ImageMemoryBarriers[0].srcAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[0].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[0].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[0].subresourceRange.levelCount = 1;
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = AverageLuminanceTexture;
		ImageMemoryBarriers[1].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = SceneLuminanceTexturesViews[3];
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		DescriptorImageInfos[1].imageView = AverageLuminanceTextureView;
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = LuminancePassSets[4][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = LuminancePassSets[4][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminanceAvgPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, LuminancePassPipelineLayout, 0, 1, &LuminancePassSets[4][CurrentFrameIndex], 0, nullptr);

		vkCmdDispatch(CommandBuffers[CurrentFrameIndex], 1, 1, 1);
	}
	
	// ===============================================================================================================
	
	// ===============================================================================================================
	
	{
		VkImageMemoryBarrier ImageMemoryBarriers[2];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = BloomTextures[0][0];
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, ImageMemoryBarriers);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = BloomTexturesFrameBuffers[0][0];
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = BloomRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

		VkViewport Viewport;
		Viewport.height = float(ResolutionHeight);
		Viewport.maxDepth = 1.0f;
		Viewport.minDepth = 0.0f;
		Viewport.width = float(ResolutionWidth);
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;

		vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

		VkRect2D ScissorRect;
		ScissorRect.extent.height = ResolutionHeight;
		ScissorRect.extent.width = ResolutionWidth;
		ScissorRect.offset.x = 0;
		ScissorRect.offset.y = 0;

		vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

		VkDescriptorImageInfo DescriptorImageInfos[2];
		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = ResolvedHDRSceneColorTextureView;
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[1].imageView = SceneLuminanceTexturesViews[0];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;
		
		VkWriteDescriptorSet WriteDescriptorSets[2];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = BloomPassSets1[0][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = BloomPassSets1[0][CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BrightPassPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPassPipelineLayout, 0, 1, &BloomPassSets1[0][CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = BloomTextures[0][0];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = BloomTextures[1][0];
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = BloomTexturesFrameBuffers[1][0];
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = BloomRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

		Viewport.height = float(ResolutionHeight);
		Viewport.maxDepth = 1.0f;
		Viewport.minDepth = 0.0f;
		Viewport.width = float(ResolutionWidth);
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;

		vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

		ScissorRect.extent.height = ResolutionHeight;
		ScissorRect.extent.width = ResolutionWidth;
		ScissorRect.offset.x = 0;
		ScissorRect.offset.y = 0;

		vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = BloomTexturesViews[0][0];
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;

		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = BloomPassSets1[1][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 1, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, HorizontalBlurPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPassPipelineLayout, 0, 1, &BloomPassSets1[1][CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = BloomTextures[1][0];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = BloomTextures[2][0];
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = BloomTexturesFrameBuffers[2][0];
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = BloomRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

		Viewport.height = float(ResolutionHeight);
		Viewport.maxDepth = 1.0f;
		Viewport.minDepth = 0.0f;
		Viewport.width = float(ResolutionWidth);
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;

		vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

		ScissorRect.extent.height = ResolutionHeight;
		ScissorRect.extent.width = ResolutionWidth;
		ScissorRect.offset.x = 0;
		ScissorRect.offset.y = 0;

		vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = BloomTexturesViews[1][0];
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;

		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = BloomPassSets1[2][CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 1, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, VerticalBlurPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPassPipelineLayout, 0, 1, &BloomPassSets1[2][CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

		for (int i = 1; i < 7; i++)
		{
			VkImageMemoryBarrier ImageMemoryBarriers[2];
			ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			ImageMemoryBarriers[0].image = BloomTextures[0][i];
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

			vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, ImageMemoryBarriers);

			VkRenderPassBeginInfo RenderPassBeginInfo;
			RenderPassBeginInfo.clearValueCount = 0;
			RenderPassBeginInfo.framebuffer = BloomTexturesFrameBuffers[0][i];
			RenderPassBeginInfo.pClearValues = nullptr;
			RenderPassBeginInfo.pNext = nullptr;
			RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight >> i;
			RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth >> i;
			RenderPassBeginInfo.renderArea.offset.x = 0;
			RenderPassBeginInfo.renderArea.offset.y = 0;
			RenderPassBeginInfo.renderPass = BloomRenderPass;
			RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

			vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

			VkViewport Viewport;
			Viewport.height = float(ResolutionHeight >> i);
			Viewport.maxDepth = 1.0f;
			Viewport.minDepth = 0.0f;
			Viewport.width = float(ResolutionWidth >> i);
			Viewport.x = 0.0f;
			Viewport.y = 0.0f;

			vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

			VkRect2D ScissorRect;
			ScissorRect.extent.height = ResolutionHeight >> i;
			ScissorRect.extent.width = ResolutionWidth >> i;
			ScissorRect.offset.x = 0;
			ScissorRect.offset.y = 0;

			vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

			VkDescriptorImageInfo DescriptorImageInfos[2];
			DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DescriptorImageInfos[0].imageView = BloomTexturesViews[0][i - 1];
			DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
			DescriptorImageInfos[1].imageLayout = (VkImageLayout)0;
			DescriptorImageInfos[1].imageView = VK_NULL_HANDLE;
			DescriptorImageInfos[1].sampler = BiLinearSampler;

			VkWriteDescriptorSet WriteDescriptorSets[2];
			WriteDescriptorSets[0].descriptorCount = 1;
			WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			WriteDescriptorSets[0].dstArrayElement = 0;
			WriteDescriptorSets[0].dstBinding = 0;
			WriteDescriptorSets[0].dstSet = BloomPassSets2[i - 1][0][CurrentFrameIndex];
			WriteDescriptorSets[0].pBufferInfo = nullptr;
			WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
			WriteDescriptorSets[0].pNext = nullptr;
			WriteDescriptorSets[0].pTexelBufferView = nullptr;
			WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSets[1].descriptorCount = 1;
			WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
			WriteDescriptorSets[1].dstArrayElement = 0;
			WriteDescriptorSets[1].dstBinding = 2;
			WriteDescriptorSets[1].dstSet = BloomPassSets2[i - 1][0][CurrentFrameIndex];
			WriteDescriptorSets[1].pBufferInfo = nullptr;
			WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
			WriteDescriptorSets[1].pNext = nullptr;
			WriteDescriptorSets[1].pTexelBufferView = nullptr;
			WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

			vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, DownSamplePipeline);

			vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPassPipelineLayout, 0, 1, &BloomPassSets2[i - 1][0][CurrentFrameIndex], 0, nullptr);

			vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

			vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

			ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
			ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			ImageMemoryBarriers[0].image = BloomTextures[0][i];
			ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
			ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			ImageMemoryBarriers[1].image = BloomTextures[1][i];
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

			vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

			RenderPassBeginInfo.clearValueCount = 0;
			RenderPassBeginInfo.framebuffer = BloomTexturesFrameBuffers[1][i];
			RenderPassBeginInfo.pClearValues = nullptr;
			RenderPassBeginInfo.pNext = nullptr;
			RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight >> i;
			RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth >> i;
			RenderPassBeginInfo.renderArea.offset.x = 0;
			RenderPassBeginInfo.renderArea.offset.y = 0;
			RenderPassBeginInfo.renderPass = BloomRenderPass;
			RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

			vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

			Viewport.height = float(ResolutionHeight >> i);
			Viewport.maxDepth = 1.0f;
			Viewport.minDepth = 0.0f;
			Viewport.width = float(ResolutionWidth >> i);
			Viewport.x = 0.0f;
			Viewport.y = 0.0f;

			vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

			ScissorRect.extent.height = ResolutionHeight >> i;
			ScissorRect.extent.width = ResolutionWidth >> i;
			ScissorRect.offset.x = 0;
			ScissorRect.offset.y = 0;

			vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

			DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DescriptorImageInfos[0].imageView = BloomTexturesViews[0][i];
			DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;

			WriteDescriptorSets[0].descriptorCount = 1;
			WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			WriteDescriptorSets[0].dstArrayElement = 0;
			WriteDescriptorSets[0].dstBinding = 0;
			WriteDescriptorSets[0].dstSet = BloomPassSets2[i - 1][1][CurrentFrameIndex];
			WriteDescriptorSets[0].pBufferInfo = nullptr;
			WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
			WriteDescriptorSets[0].pNext = nullptr;
			WriteDescriptorSets[0].pTexelBufferView = nullptr;
			WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			vkUpdateDescriptorSets(Device, 1, WriteDescriptorSets, 0, nullptr);

			vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, HorizontalBlurPipeline);

			vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPassPipelineLayout, 0, 1, &BloomPassSets2[i - 1][1][CurrentFrameIndex], 0, nullptr);

			vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

			vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);

			ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
			ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			ImageMemoryBarriers[0].image = BloomTextures[1][i];
			ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
			ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			ImageMemoryBarriers[1].image = BloomTextures[2][i];
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

			vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 2, ImageMemoryBarriers);

			RenderPassBeginInfo.clearValueCount = 0;
			RenderPassBeginInfo.framebuffer = BloomTexturesFrameBuffers[2][i];
			RenderPassBeginInfo.pClearValues = nullptr;
			RenderPassBeginInfo.pNext = nullptr;
			RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight >> i;
			RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth >> i;
			RenderPassBeginInfo.renderArea.offset.x = 0;
			RenderPassBeginInfo.renderArea.offset.y = 0;
			RenderPassBeginInfo.renderPass = BloomRenderPass;
			RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

			vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

			Viewport.height = float(ResolutionHeight >> i);
			Viewport.maxDepth = 1.0f;
			Viewport.minDepth = 0.0f;
			Viewport.width = float(ResolutionWidth >> i);
			Viewport.x = 0.0f;
			Viewport.y = 0.0f;

			vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

			ScissorRect.extent.height = ResolutionHeight >> i;
			ScissorRect.extent.width = ResolutionWidth >> i;
			ScissorRect.offset.x = 0;
			ScissorRect.offset.y = 0;

			vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

			DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DescriptorImageInfos[0].imageView = BloomTexturesViews[1][i];
			DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;

			WriteDescriptorSets[0].descriptorCount = 1;
			WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			WriteDescriptorSets[0].dstArrayElement = 0;
			WriteDescriptorSets[0].dstBinding = 0;
			WriteDescriptorSets[0].dstSet = BloomPassSets2[i - 1][2][CurrentFrameIndex];
			WriteDescriptorSets[0].pBufferInfo = nullptr;
			WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
			WriteDescriptorSets[0].pNext = nullptr;
			WriteDescriptorSets[0].pTexelBufferView = nullptr;
			WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			vkUpdateDescriptorSets(Device, 1, WriteDescriptorSets, 0, nullptr);

			vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, VerticalBlurPipeline);

			vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPassPipelineLayout, 0, 1, &BloomPassSets2[i - 1][2][CurrentFrameIndex], 0, nullptr);

			vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

			vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
		}

		for (int i = 5; i >= 0; i--)
		{
			VkImageMemoryBarrier ImageMemoryBarriers[2];
			ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
			ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			ImageMemoryBarriers[0].image = BloomTextures[2][i + 1];
			ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

			vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, ImageMemoryBarriers);

			VkRenderPassBeginInfo RenderPassBeginInfo;
			RenderPassBeginInfo.clearValueCount = 0;
			RenderPassBeginInfo.framebuffer = BloomTexturesFrameBuffers[2][i];
			RenderPassBeginInfo.pClearValues = nullptr;
			RenderPassBeginInfo.pNext = nullptr;
			RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight >> i;
			RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth >> i;
			RenderPassBeginInfo.renderArea.offset.x = 0;
			RenderPassBeginInfo.renderArea.offset.y = 0;
			RenderPassBeginInfo.renderPass = BloomRenderPass;
			RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

			vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

			VkViewport Viewport;
			Viewport.height = float(ResolutionHeight >> i);
			Viewport.maxDepth = 1.0f;
			Viewport.minDepth = 0.0f;
			Viewport.width = float(ResolutionWidth >> i);
			Viewport.x = 0.0f;
			Viewport.y = 0.0f;

			vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

			VkRect2D ScissorRect;
			ScissorRect.extent.height = ResolutionHeight >> i;
			ScissorRect.extent.width = ResolutionWidth >> i;
			ScissorRect.offset.x = 0;
			ScissorRect.offset.y = 0;

			vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

			VkDescriptorImageInfo DescriptorImageInfos[2];
			DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			DescriptorImageInfos[0].imageView = BloomTexturesViews[2][i + 1];
			DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
			DescriptorImageInfos[1].imageLayout = (VkImageLayout)0;
			DescriptorImageInfos[1].imageView = VK_NULL_HANDLE;
			DescriptorImageInfos[1].sampler = BiLinearSampler;

			VkWriteDescriptorSet WriteDescriptorSets[2];
			WriteDescriptorSets[0].descriptorCount = 1;
			WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			WriteDescriptorSets[0].dstArrayElement = 0;
			WriteDescriptorSets[0].dstBinding = 0;
			WriteDescriptorSets[0].dstSet = BloomPassSets3[5 - i][CurrentFrameIndex];
			WriteDescriptorSets[0].pBufferInfo = nullptr;
			WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
			WriteDescriptorSets[0].pNext = nullptr;
			WriteDescriptorSets[0].pTexelBufferView = nullptr;
			WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSets[1].descriptorCount = 1;
			WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER;
			WriteDescriptorSets[1].dstArrayElement = 0;
			WriteDescriptorSets[1].dstBinding = 2;
			WriteDescriptorSets[1].dstSet = BloomPassSets3[5 - i][CurrentFrameIndex];
			WriteDescriptorSets[1].pBufferInfo = nullptr;
			WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
			WriteDescriptorSets[1].pNext = nullptr;
			WriteDescriptorSets[1].pTexelBufferView = nullptr;
			WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

			vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, UpSampleWithAddBlendPipeline);

			vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, BloomPassPipelineLayout, 0, 1, &BloomPassSets3[5 - i][CurrentFrameIndex], 0, nullptr);

			vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

			vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
		}
	}

	// ===============================================================================================================

	{
		VkImageMemoryBarrier ImageMemoryBarriers[3];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[0].image = BloomTextures[2][0];
		ImageMemoryBarriers[0].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		ImageMemoryBarriers[1].dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[1].image = ToneMappedImageTexture;
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
		ImageMemoryBarriers[2].dstAccessMask = VkAccessFlagBits::VK_ACCESS_SHADER_READ_BIT;
		ImageMemoryBarriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].image = HDRSceneColorTexture;
		ImageMemoryBarriers[2].newLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		ImageMemoryBarriers[2].oldLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageMemoryBarriers[2].pNext = nullptr;
		ImageMemoryBarriers[2].srcAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		ImageMemoryBarriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarriers[2].sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImageMemoryBarriers[2].subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
		ImageMemoryBarriers[2].subresourceRange.baseArrayLayer = 0;
		ImageMemoryBarriers[2].subresourceRange.baseMipLevel = 0;
		ImageMemoryBarriers[2].subresourceRange.layerCount = 1;
		ImageMemoryBarriers[2].subresourceRange.levelCount = 1;

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 3, ImageMemoryBarriers);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = ToneMappedImageFrameBuffer;
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = HDRToneMappingRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);

		VkViewport Viewport;
		Viewport.height = float(ResolutionHeight);
		Viewport.maxDepth = 1.0f;
		Viewport.minDepth = 0.0f;
		Viewport.width = float(ResolutionWidth);
		Viewport.x = 0.0f;
		Viewport.y = 0.0f;

		vkCmdSetViewport(CommandBuffers[CurrentFrameIndex], 0, 1, &Viewport);

		VkRect2D ScissorRect;
		ScissorRect.extent.height = ResolutionHeight;
		ScissorRect.extent.width = ResolutionWidth;
		ScissorRect.offset.x = 0;
		ScissorRect.offset.y = 0;

		vkCmdSetScissor(CommandBuffers[CurrentFrameIndex], 0, 1, &ScissorRect);

		VkDescriptorImageInfo DescriptorImageInfos[2];
		DescriptorImageInfos[0].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[0].imageView = HDRSceneColorTextureView;
		DescriptorImageInfos[0].sampler = VK_NULL_HANDLE;
		DescriptorImageInfos[1].imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		DescriptorImageInfos[1].imageView = BloomTexturesViews[2][0];
		DescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

		VkWriteDescriptorSet WriteDescriptorSets[2];
		WriteDescriptorSets[0].descriptorCount = 1;
		WriteDescriptorSets[0].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[0].dstArrayElement = 0;
		WriteDescriptorSets[0].dstBinding = 0;
		WriteDescriptorSets[0].dstSet = HDRToneMappingSets[CurrentFrameIndex];
		WriteDescriptorSets[0].pBufferInfo = nullptr;
		WriteDescriptorSets[0].pImageInfo = &DescriptorImageInfos[0];
		WriteDescriptorSets[0].pNext = nullptr;
		WriteDescriptorSets[0].pTexelBufferView = nullptr;
		WriteDescriptorSets[0].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSets[1].descriptorCount = 1;
		WriteDescriptorSets[1].descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		WriteDescriptorSets[1].dstArrayElement = 0;
		WriteDescriptorSets[1].dstBinding = 1;
		WriteDescriptorSets[1].dstSet = HDRToneMappingSets[CurrentFrameIndex];
		WriteDescriptorSets[1].pBufferInfo = nullptr;
		WriteDescriptorSets[1].pImageInfo = &DescriptorImageInfos[1];
		WriteDescriptorSets[1].pNext = nullptr;
		WriteDescriptorSets[1].pTexelBufferView = nullptr;
		WriteDescriptorSets[1].sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, 2, WriteDescriptorSets, 0, nullptr);

		vkCmdBindPipeline(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, HDRToneMappingPipeline);

		vkCmdBindDescriptorSets(CommandBuffers[CurrentFrameIndex], VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, HDRToneMappingPipelineLayout, 0, 1, &HDRToneMappingSets[CurrentFrameIndex], 0, nullptr);

		vkCmdDraw(CommandBuffers[CurrentFrameIndex], 4, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
	}

	// ===============================================================================================================
	
	{
		VkImageMemoryBarrier ImageMemoryBarriers[1];
		ImageMemoryBarriers[0].dstAccessMask = VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
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

		vkCmdPipelineBarrier(CommandBuffers[CurrentFrameIndex], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, ImageMemoryBarriers);

		VkRenderPassBeginInfo RenderPassBeginInfo;
		RenderPassBeginInfo.clearValueCount = 0;
		RenderPassBeginInfo.framebuffer = BackBufferFrameBuffers[CurrentFrameIndex];
		RenderPassBeginInfo.pClearValues = nullptr;
		RenderPassBeginInfo.pNext = nullptr;
		RenderPassBeginInfo.renderArea.extent.height = ResolutionHeight;
		RenderPassBeginInfo.renderArea.extent.width = ResolutionWidth;
		RenderPassBeginInfo.renderArea.offset.x = 0;
		RenderPassBeginInfo.renderArea.offset.y = 0;
		RenderPassBeginInfo.renderPass = BackBufferResolveRenderPass;
		RenderPassBeginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(CommandBuffers[CurrentFrameIndex], &RenderPassBeginInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(CommandBuffers[CurrentFrameIndex]);
	}
	// ===============================================================================================================

	VkImageMemoryBarrier ImageMemoryBarriers[1];
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