// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RenderDeviceVulkan.h"

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
VkBool32 VKAPI_PTR vkDebugUtilsMessengerCallbackEXT(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
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

void RenderDeviceVulkan::InitDevice()
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

	VkPhysicalDeviceProperties PhysicalDeviceProperties;

	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);

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

	/*VkMemoryPropertyFlags DefaultHeapFlags = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkMemoryPropertyFlags UploadHeapFlags = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	VkMemoryPropertyFlags ReadbackHeapFlags = VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

	for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & DefaultHeapFlags) == DefaultHeapFlags) DefaultMemoryHeapIndex = i;
		if ((PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & UploadHeapFlags) == UploadHeapFlags) UploadMemoryHeapIndex = i;
		if ((PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & ReadbackHeapFlags) == ReadbackHeapFlags) ReadbackMemoryHeapIndex = i;
	}*/

	for (uint32_t i = 0; i < PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
			!(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
			DefaultMemoryHeapIndex = i;

		if (!(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
			(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
			!(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
			UploadMemoryHeapIndex = i;

		if (!(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
			(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
			(PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
			ReadbackMemoryHeapIndex = i;
	}

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

	SIZE_T FullScreenQuadVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.FullScreenQuad");
	ScopedMemoryBlockArray<BYTE> FullScreenQuadVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FullScreenQuadVertexShaderByteCodeLength);
	Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.FullScreenQuad", FullScreenQuadVertexShaderByteCodeData);
	
	VkShaderModule FullScreenQuadShaderModule;

	VkShaderModuleCreateInfo ShaderModuleCreateInfo;
	ShaderModuleCreateInfo.codeSize = FullScreenQuadVertexShaderByteCodeLength;
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
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

		MemoryAllocateInfo.allocationSize = TEXTURE_MEMORY_HEAP_SIZE;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
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
		MemoryAllocateInfo.memoryTypeIndex = UploadMemoryHeapIndex;
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

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers[1]));

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, GBufferTextures[0], &MemoryRequirements);

		VkDeviceSize GBufferTexture0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = GBufferTexture0Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, GBufferTextures[1], &MemoryRequirements);

		VkDeviceSize GBufferTexture1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = GBufferTexture1Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, DepthBufferTexture, &MemoryRequirements);

		VkDeviceSize DepthBufferTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = DepthBufferTextureOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUConstantBuffer, &MemoryRequirements);

		VkDeviceSize GPUConstantBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		GPUConstantBufferOffset = (GPUConstantBufferOffset == 0) ? 0 : GPUConstantBufferOffset + (PhysicalDeviceProperties.limits.bufferImageGranularity - GPUConstantBufferOffset % PhysicalDeviceProperties.limits.bufferImageGranularity);
		MemoryAllocateInfo.allocationSize = GPUConstantBufferOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory1));

		SAFE_VK(vkBindImageMemory(Device, GBufferTextures[0], GPUMemory1, GBufferTexture0Offset));
		SAFE_VK(vkBindImageMemory(Device, GBufferTextures[1], GPUMemory1, GBufferTexture1Offset));
		SAFE_VK(vkBindImageMemory(Device, DepthBufferTexture, GPUMemory1, DepthBufferTextureOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPUConstantBuffer, GPUMemory1, GPUConstantBufferOffset));

		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = UploadMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers[0], &MemoryRequirements);

		VkDeviceSize CPUConstantBuffer0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUConstantBuffer0Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers[1], &MemoryRequirements);

		VkDeviceSize CPUConstantBuffer1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUConstantBuffer1Offset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUMemory1));

		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers[0], CPUMemory1, CPUConstantBuffer0Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers[1], CPUMemory1, CPUConstantBuffer1Offset));

		ConstantBuffersOffets1[0] = CPUConstantBuffer0Offset;
		ConstantBuffersOffets1[1] = CPUConstantBuffer1Offset;

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

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, ResolvedDepthBufferTexture, &MemoryRequirements);

		VkDeviceSize ResolvedDepthBufferTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ResolvedDepthBufferTextureOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory2));

		SAFE_VK(vkBindImageMemory(Device, ResolvedDepthBufferTexture, GPUMemory2, ResolvedDepthBufferTextureOffset));

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

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, OcclusionBufferTexture, &MemoryRequirements);

		VkDeviceSize OcclusionBufferTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = OcclusionBufferTextureOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory3));

		SAFE_VK(vkBindImageMemory(Device, OcclusionBufferTexture, GPUMemory3, OcclusionBufferTextureOffset));

		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = ReadbackMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		vkGetBufferMemoryRequirements(Device, OcclusionBufferReadbackBuffers[0], &MemoryRequirements);

		VkDeviceSize OcclusionBufferReadbackBuffer0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = OcclusionBufferReadbackBuffer0Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, OcclusionBufferReadbackBuffers[1], &MemoryRequirements);

		VkDeviceSize OcclusionBufferReadbackBuffer1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = OcclusionBufferReadbackBuffer1Offset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUMemory3));

		SAFE_VK(vkBindBufferMemory(Device, OcclusionBufferReadbackBuffers[0], CPUMemory3, OcclusionBufferReadbackBuffer0Offset));
		SAFE_VK(vkBindBufferMemory(Device, OcclusionBufferReadbackBuffers[1], CPUMemory3, OcclusionBufferReadbackBuffer1Offset));

		OcclusionBuffersOffsets[0] = OcclusionBufferReadbackBuffer0Offset;
		OcclusionBuffersOffsets[1] = OcclusionBufferReadbackBuffer1Offset;

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

		SIZE_T OcclusionBufferPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.OcclusionBuffer");
		ScopedMemoryBlockArray<BYTE> OcclusionBufferPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(OcclusionBufferPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.OcclusionBuffer", OcclusionBufferPixelShaderByteCodeData);

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = OcclusionBufferPixelShaderByteCodeLength;
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

		vkDestroyShaderModule(Device, OcclusionBufferShaderModule, nullptr);
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

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[0][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[0][1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[1][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[1][1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[2][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[2][1]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[3][0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUConstantBuffers2[3][1]));

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, CascadedShadowMapTextures[0], &MemoryRequirements);

		VkDeviceSize CascadedShadowMapTexture0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CascadedShadowMapTexture0Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, CascadedShadowMapTextures[1], &MemoryRequirements);

		VkDeviceSize CascadedShadowMapTexture1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CascadedShadowMapTexture1Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, CascadedShadowMapTextures[2], &MemoryRequirements);

		VkDeviceSize CascadedShadowMapTexture2Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CascadedShadowMapTexture2Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, CascadedShadowMapTextures[3], &MemoryRequirements);

		VkDeviceSize CascadedShadowMapTexture3Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CascadedShadowMapTexture3Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUConstantBuffers2[0], &MemoryRequirements);

		VkDeviceSize ConstantBuffer0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		ConstantBuffer0Offset = (ConstantBuffer0Offset == 0) ? 0 : ConstantBuffer0Offset + (PhysicalDeviceProperties.limits.bufferImageGranularity - ConstantBuffer0Offset % PhysicalDeviceProperties.limits.bufferImageGranularity);
		MemoryAllocateInfo.allocationSize = ConstantBuffer0Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUConstantBuffers2[1], &MemoryRequirements);

		VkDeviceSize ConstantBuffer1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer1Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUConstantBuffers2[2], &MemoryRequirements);

		VkDeviceSize ConstantBuffer2Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer2Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUConstantBuffers2[3], &MemoryRequirements);

		VkDeviceSize ConstantBuffer3Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer3Offset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory4));

		SAFE_VK(vkBindImageMemory(Device, CascadedShadowMapTextures[0], GPUMemory4, CascadedShadowMapTexture0Offset));
		SAFE_VK(vkBindImageMemory(Device, CascadedShadowMapTextures[1], GPUMemory4, CascadedShadowMapTexture1Offset));
		SAFE_VK(vkBindImageMemory(Device, CascadedShadowMapTextures[2], GPUMemory4, CascadedShadowMapTexture2Offset));
		SAFE_VK(vkBindImageMemory(Device, CascadedShadowMapTextures[3], GPUMemory4, CascadedShadowMapTexture3Offset));
		SAFE_VK(vkBindBufferMemory(Device, GPUConstantBuffers2[0], GPUMemory4, ConstantBuffer0Offset));
		SAFE_VK(vkBindBufferMemory(Device, GPUConstantBuffers2[1], GPUMemory4, ConstantBuffer1Offset));
		SAFE_VK(vkBindBufferMemory(Device, GPUConstantBuffers2[2], GPUMemory4, ConstantBuffer2Offset));
		SAFE_VK(vkBindBufferMemory(Device, GPUConstantBuffers2[3], GPUMemory4, ConstantBuffer3Offset));

		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = UploadMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[0][0], &MemoryRequirements);

		VkDeviceSize ConstantBuffer00Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer00Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[0][1], &MemoryRequirements);

		VkDeviceSize ConstantBuffer01Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer01Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[1][0], &MemoryRequirements);

		VkDeviceSize ConstantBuffer10Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer10Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[1][1], &MemoryRequirements);

		VkDeviceSize ConstantBuffer11Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer11Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[2][0], &MemoryRequirements);

		VkDeviceSize ConstantBuffer20Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer20Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[2][1], &MemoryRequirements);

		VkDeviceSize ConstantBuffer21Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer21Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[3][0], &MemoryRequirements);

		VkDeviceSize ConstantBuffer30Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer30Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUConstantBuffers2[3][1], &MemoryRequirements);

		VkDeviceSize ConstantBuffer31Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ConstantBuffer31Offset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUMemory4));

		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[0][0], CPUMemory4, ConstantBuffer00Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[0][1], CPUMemory4, ConstantBuffer01Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[1][0], CPUMemory4, ConstantBuffer10Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[1][1], CPUMemory4, ConstantBuffer11Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[2][0], CPUMemory4, ConstantBuffer20Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[2][1], CPUMemory4, ConstantBuffer21Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[3][0], CPUMemory4, ConstantBuffer30Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUConstantBuffers2[3][1], CPUMemory4, ConstantBuffer31Offset));

		ConstantBuffersOffets2[0][0] = ConstantBuffer00Offset;
		ConstantBuffersOffets2[0][1] = ConstantBuffer01Offset;
		ConstantBuffersOffets2[1][0] = ConstantBuffer10Offset;
		ConstantBuffersOffets2[1][1] = ConstantBuffer11Offset;
		ConstantBuffersOffets2[2][0] = ConstantBuffer20Offset;
		ConstantBuffersOffets2[2][1] = ConstantBuffer21Offset;
		ConstantBuffersOffets2[3][0] = ConstantBuffer30Offset;
		ConstantBuffersOffets2[3][1] = ConstantBuffer31Offset;

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

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUShadowResolveConstantBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUShadowResolveConstantBuffers[1]));

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, ShadowMaskTexture, &MemoryRequirements);

		VkDeviceSize ShadowMaskTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ShadowMaskTextureOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUShadowResolveConstantBuffer, &MemoryRequirements);

		VkDeviceSize GPUConstantBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		GPUConstantBufferOffset = (GPUConstantBufferOffset == 0) ? 0 : GPUConstantBufferOffset + (PhysicalDeviceProperties.limits.bufferImageGranularity - GPUConstantBufferOffset % PhysicalDeviceProperties.limits.bufferImageGranularity);
		MemoryAllocateInfo.allocationSize = GPUConstantBufferOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory5));

		SAFE_VK(vkBindImageMemory(Device, ShadowMaskTexture, GPUMemory5, ShadowMaskTextureOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPUShadowResolveConstantBuffer, GPUMemory5, GPUConstantBufferOffset));

		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = UploadMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		vkGetBufferMemoryRequirements(Device, CPUShadowResolveConstantBuffers[0], &MemoryRequirements);

		VkDeviceSize CPUConstantBuffer0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUConstantBuffer0Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUShadowResolveConstantBuffers[1], &MemoryRequirements);

		VkDeviceSize CPUConstantBuffer1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUConstantBuffer1Offset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUMemory5));

		SAFE_VK(vkBindBufferMemory(Device, CPUShadowResolveConstantBuffers[0], CPUMemory5, CPUConstantBuffer0Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUShadowResolveConstantBuffers[1], CPUMemory5, CPUConstantBuffer1Offset));

		ConstantBuffersOffets3[0] = CPUConstantBuffer0Offset;
		ConstantBuffersOffets3[1] = CPUConstantBuffer1Offset;

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

		SIZE_T ShadowResolvePixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.ShadowResolve");
		ScopedMemoryBlockArray<BYTE> ShadowResolvePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ShadowResolvePixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.ShadowResolve", ShadowResolvePixelShaderByteCodeData);

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = ShadowResolvePixelShaderByteCodeLength;
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

		vkDestroyShaderModule(Device, ShadowResolveShaderModule, nullptr);
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

		BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUDeferredLightingConstantBuffers[0]));
		SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &CPUDeferredLightingConstantBuffers[1]));

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

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, HDRSceneColorTexture, &MemoryRequirements);

		VkDeviceSize HDRSceneColorTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = HDRSceneColorTextureOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUDeferredLightingConstantBuffer, &MemoryRequirements);

		VkDeviceSize GPUConstantBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		GPUConstantBufferOffset = (GPUConstantBufferOffset == 0) ? 0 : GPUConstantBufferOffset + (PhysicalDeviceProperties.limits.bufferImageGranularity - GPUConstantBufferOffset % PhysicalDeviceProperties.limits.bufferImageGranularity);
		MemoryAllocateInfo.allocationSize = GPUConstantBufferOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPULightClustersBuffer, &MemoryRequirements);

		VkDeviceSize GPULightClustersBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = GPULightClustersBufferOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPULightIndicesBuffer, &MemoryRequirements);

		VkDeviceSize GPULightIndicesBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = GPULightIndicesBufferOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUPointLightsBuffer, &MemoryRequirements);

		VkDeviceSize GPUPointLightsBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = GPUPointLightsBufferOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory6));

		SAFE_VK(vkBindImageMemory(Device, HDRSceneColorTexture, GPUMemory6, HDRSceneColorTextureOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPUDeferredLightingConstantBuffer, GPUMemory6, GPUConstantBufferOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPULightClustersBuffer, GPUMemory6, GPULightClustersBufferOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPULightIndicesBuffer, GPUMemory6, GPULightIndicesBufferOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPUPointLightsBuffer, GPUMemory6, GPUPointLightsBufferOffset));

		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = UploadMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		vkGetBufferMemoryRequirements(Device, CPUDeferredLightingConstantBuffers[0], &MemoryRequirements);

		VkDeviceSize CPUConstantBuffer0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUConstantBuffer0Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUDeferredLightingConstantBuffers[1], &MemoryRequirements);

		VkDeviceSize CPUConstantBuffer1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUConstantBuffer1Offset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPULightClustersBuffers[0], &MemoryRequirements);

		VkDeviceSize CPULightClustersBufferOffset0 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPULightClustersBufferOffset0 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPULightClustersBuffers[1], &MemoryRequirements);

		VkDeviceSize CPULightClustersBufferOffset1 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPULightClustersBufferOffset1 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPULightIndicesBuffers[0], &MemoryRequirements);

		VkDeviceSize CPULightIndicesBufferOffset0 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPULightIndicesBufferOffset0 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPULightIndicesBuffers[1], &MemoryRequirements);

		VkDeviceSize CPULightIndicesBufferOffset1 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPULightIndicesBufferOffset1 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUPointLightsBuffers[0], &MemoryRequirements);

		VkDeviceSize CPUPointLightsBufferOffset0 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUPointLightsBufferOffset0 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUPointLightsBuffers[1], &MemoryRequirements);

		VkDeviceSize CPUPointLightsBufferOffset1 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUPointLightsBufferOffset1 + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUMemory6));

		SAFE_VK(vkBindBufferMemory(Device, CPUDeferredLightingConstantBuffers[0], CPUMemory6, CPUConstantBuffer0Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPUDeferredLightingConstantBuffers[1], CPUMemory6, CPUConstantBuffer1Offset));
		SAFE_VK(vkBindBufferMemory(Device, CPULightClustersBuffers[0], CPUMemory6, CPULightClustersBufferOffset0));
		SAFE_VK(vkBindBufferMemory(Device, CPULightClustersBuffers[1], CPUMemory6, CPULightClustersBufferOffset1));
		SAFE_VK(vkBindBufferMemory(Device, CPULightIndicesBuffers[0], CPUMemory6, CPULightIndicesBufferOffset0));
		SAFE_VK(vkBindBufferMemory(Device, CPULightIndicesBuffers[1], CPUMemory6, CPULightIndicesBufferOffset1));
		SAFE_VK(vkBindBufferMemory(Device, CPUPointLightsBuffers[0], CPUMemory6, CPUPointLightsBufferOffset0));
		SAFE_VK(vkBindBufferMemory(Device, CPUPointLightsBuffers[1], CPUMemory6, CPUPointLightsBufferOffset1));

		ConstantBuffersOffets4[0] = CPUConstantBuffer0Offset;
		ConstantBuffersOffets4[1] = CPUConstantBuffer1Offset;

		DynamicBuffersOffsets[0][0] = CPULightClustersBufferOffset0;
		DynamicBuffersOffsets[0][1] = CPULightClustersBufferOffset1;
		DynamicBuffersOffsets[1][0] = CPULightIndicesBufferOffset0;
		DynamicBuffersOffsets[1][1] = CPULightIndicesBufferOffset1;
		DynamicBuffersOffsets[2][0] = CPUPointLightsBufferOffset0;
		DynamicBuffersOffsets[2][1] = CPUPointLightsBufferOffset1;

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

		SIZE_T DeferredLightingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.DeferredLighting");
		ScopedMemoryBlockArray<BYTE> DeferredLightingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(DeferredLightingPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.DeferredLighting", DeferredLightingPixelShaderByteCodeData);

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = DeferredLightingPixelShaderByteCodeLength;
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

		vkDestroyShaderModule(Device, DeferredLightingShaderModule, nullptr);
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

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetBufferMemoryRequirements(Device, SkyVertexBuffer, &MemoryRequirements);

		VkDeviceSize SkyVertexBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SkyVertexBufferOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, SkyIndexBuffer, &MemoryRequirements);

		VkDeviceSize SkyIndexBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SkyIndexBufferOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, SunVertexBuffer, &MemoryRequirements);

		VkDeviceSize SunVertexBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SunVertexBufferOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, SunIndexBuffer, &MemoryRequirements);

		VkDeviceSize SunIndexBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SunIndexBufferOffset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, SkyTexture, &MemoryRequirements);

		VkDeviceSize SkyTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		SkyTextureOffset = (SkyTextureOffset == 0) ? 0 : SkyTextureOffset + (PhysicalDeviceProperties.limits.bufferImageGranularity - SkyTextureOffset % PhysicalDeviceProperties.limits.bufferImageGranularity);
		MemoryAllocateInfo.allocationSize = SkyTextureOffset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, SunTexture, &MemoryRequirements);

		VkDeviceSize SunTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SunTextureOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUSkyConstantBuffer, &MemoryRequirements);

		VkDeviceSize GPUSkyConstantBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		GPUSkyConstantBufferOffset = (GPUSkyConstantBufferOffset == 0) ? 0 : GPUSkyConstantBufferOffset + (PhysicalDeviceProperties.limits.bufferImageGranularity - GPUSkyConstantBufferOffset % PhysicalDeviceProperties.limits.bufferImageGranularity);
		MemoryAllocateInfo.allocationSize = GPUSkyConstantBufferOffset + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, GPUSunConstantBuffer, &MemoryRequirements);

		VkDeviceSize GPUSunConstantBufferOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = GPUSunConstantBufferOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory7));

		SAFE_VK(vkBindBufferMemory(Device, SkyVertexBuffer, GPUMemory7, SkyVertexBufferOffset));
		SAFE_VK(vkBindBufferMemory(Device, SkyIndexBuffer, GPUMemory7, SkyIndexBufferOffset));
		SAFE_VK(vkBindBufferMemory(Device, SunVertexBuffer, GPUMemory7, SunVertexBufferOffset));
		SAFE_VK(vkBindBufferMemory(Device, SunIndexBuffer, GPUMemory7, SunIndexBufferOffset));
		SAFE_VK(vkBindImageMemory(Device, SkyTexture, GPUMemory7, SkyTextureOffset));
		SAFE_VK(vkBindImageMemory(Device, SunTexture, GPUMemory7, SunTextureOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPUSkyConstantBuffer, GPUMemory7, GPUSkyConstantBufferOffset));
		SAFE_VK(vkBindBufferMemory(Device, GPUSunConstantBuffer, GPUMemory7, GPUSunConstantBufferOffset));

		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = UploadMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		vkGetBufferMemoryRequirements(Device, CPUSkyConstantBuffers[0], &MemoryRequirements);

		VkDeviceSize CPUSkyConstantBufferOffset0 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUSkyConstantBufferOffset0 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUSkyConstantBuffers[1], &MemoryRequirements);

		VkDeviceSize CPUSkyConstantBufferOffset1 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUSkyConstantBufferOffset1 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUSunConstantBuffers[0], &MemoryRequirements);

		VkDeviceSize CPUSunConstantBufferOffset0 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUSunConstantBufferOffset0 + MemoryRequirements.size;

		vkGetBufferMemoryRequirements(Device, CPUSunConstantBuffers[1], &MemoryRequirements);

		VkDeviceSize CPUSunConstantBufferOffset1 = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = CPUSunConstantBufferOffset1 + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &CPUMemory7));

		SAFE_VK(vkBindBufferMemory(Device, CPUSkyConstantBuffers[0], CPUMemory7, CPUSkyConstantBufferOffset0));
		SAFE_VK(vkBindBufferMemory(Device, CPUSkyConstantBuffers[1], CPUMemory7, CPUSkyConstantBufferOffset1));
		SAFE_VK(vkBindBufferMemory(Device, CPUSunConstantBuffers[0], CPUMemory7, CPUSunConstantBufferOffset0));
		SAFE_VK(vkBindBufferMemory(Device, CPUSunConstantBuffers[1], CPUMemory7, CPUSunConstantBufferOffset1));

		ConstantBuffersOffets5[0][0] = CPUSkyConstantBufferOffset0;
		ConstantBuffersOffets5[0][1] = CPUSkyConstantBufferOffset1;
		ConstantBuffersOffets5[1][0] = CPUSunConstantBufferOffset0;
		ConstantBuffersOffets5[1][1] = CPUSunConstantBufferOffset1;

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

		SIZE_T SkyVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.SkyVertexShader");
		ScopedMemoryBlockArray<BYTE> SkyVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyVertexShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.SkyVertexShader", SkyVertexShaderByteCodeData);

		SIZE_T SkyPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.SkyPixelShader");
		ScopedMemoryBlockArray<BYTE> SkyPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SkyPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.SkyPixelShader", SkyPixelShaderByteCodeData);

		SIZE_T SunVertexShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.SunVertexShader");
		ScopedMemoryBlockArray<BYTE> SunVertexShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunVertexShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.SunVertexShader", SunVertexShaderByteCodeData);

		SIZE_T SunPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.SunPixelShader");
		ScopedMemoryBlockArray<BYTE> SunPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(SunPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.SunPixelShader", SunPixelShaderByteCodeData);

		VkShaderModule SkyVertexShaderModule, SkyPixelShaderModule, SunVertexShaderModule, SunPixelShaderModule;

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = SkyVertexShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = SkyVertexShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &SkyVertexShaderModule));

		ShaderModuleCreateInfo.codeSize = SkyPixelShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = SkyPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &SkyPixelShaderModule));

		ShaderModuleCreateInfo.codeSize = SunVertexShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = SunVertexShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &SunVertexShaderModule));

		ShaderModuleCreateInfo.codeSize = SunPixelShaderByteCodeLength;
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

		VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo;
		ZeroMemory(&PipelineMultisampleStateCreateInfo, sizeof(VkPipelineMultisampleStateCreateInfo));
		PipelineMultisampleStateCreateInfo.pNext = nullptr;
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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

		ZeroMemory(&PipelineMultisampleStateCreateInfo, sizeof(VkPipelineMultisampleStateCreateInfo));
		PipelineMultisampleStateCreateInfo.pNext = nullptr;
		PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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

		SIZE_T FogPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.Fog");
		ScopedMemoryBlockArray<BYTE> FogPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(FogPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.Fog", FogPixelShaderByteCodeData);

		ShaderModuleCreateInfo.codeSize = FogPixelShaderByteCodeLength;
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

		vkDestroyShaderModule(Device, SkyVertexShaderModule, nullptr);
		vkDestroyShaderModule(Device, SkyPixelShaderModule, nullptr);
		vkDestroyShaderModule(Device, SunVertexShaderModule, nullptr);
		vkDestroyShaderModule(Device, SunPixelShaderModule, nullptr);
		vkDestroyShaderModule(Device, FogShaderModule, nullptr);
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

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, ResolvedHDRSceneColorTexture, &MemoryRequirements);

		VkDeviceSize ResolvedHDRSceneColorTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ResolvedHDRSceneColorTextureOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory8));

		SAFE_VK(vkBindImageMemory(Device, ResolvedHDRSceneColorTexture, GPUMemory8, ResolvedHDRSceneColorTextureOffset));

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

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, SceneLuminanceTextures[0], &MemoryRequirements);

		VkDeviceSize SceneLuminanceTexture0Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SceneLuminanceTexture0Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, SceneLuminanceTextures[1], &MemoryRequirements);

		VkDeviceSize SceneLuminanceTexture1Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SceneLuminanceTexture1Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, SceneLuminanceTextures[2], &MemoryRequirements);

		VkDeviceSize SceneLuminanceTexture2Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SceneLuminanceTexture2Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, SceneLuminanceTextures[3], &MemoryRequirements);

		VkDeviceSize SceneLuminanceTexture3Offset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = SceneLuminanceTexture3Offset + MemoryRequirements.size;

		vkGetImageMemoryRequirements(Device, AverageLuminanceTexture, &MemoryRequirements);

		VkDeviceSize AverageLuminanceTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = AverageLuminanceTextureOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory9));

		SAFE_VK(vkBindImageMemory(Device, SceneLuminanceTextures[0], GPUMemory9, SceneLuminanceTexture0Offset));
		SAFE_VK(vkBindImageMemory(Device, SceneLuminanceTextures[1], GPUMemory9, SceneLuminanceTexture1Offset));
		SAFE_VK(vkBindImageMemory(Device, SceneLuminanceTextures[2], GPUMemory9, SceneLuminanceTexture2Offset));
		SAFE_VK(vkBindImageMemory(Device, SceneLuminanceTextures[3], GPUMemory9, SceneLuminanceTexture3Offset));
		SAFE_VK(vkBindImageMemory(Device, AverageLuminanceTexture, GPUMemory9, AverageLuminanceTextureOffset));

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

		SIZE_T LuminanceCalcComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.LuminanceCalc");
		ScopedMemoryBlockArray<BYTE> LuminanceCalcComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceCalcComputeShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.LuminanceCalc", LuminanceCalcComputeShaderByteCodeData);

		SIZE_T LuminanceSumComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.LuminanceSum");
		ScopedMemoryBlockArray<BYTE> LuminanceSumComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceSumComputeShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.LuminanceSum", LuminanceSumComputeShaderByteCodeData);

		SIZE_T LuminanceAvgComputeShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.LuminanceAvg");
		ScopedMemoryBlockArray<BYTE> LuminanceAvgComputeShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(LuminanceAvgComputeShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.LuminanceAvg", LuminanceAvgComputeShaderByteCodeData);

		VkShaderModule LuminanceCalcComputeShaderModule, LuminanceSumComputeShaderModule, LuminanceAvgComputeShaderModule;

		VkShaderModuleCreateInfo ShaderModuleCreateInfo;
		ShaderModuleCreateInfo.codeSize = LuminanceCalcComputeShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = LuminanceCalcComputeShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &LuminanceCalcComputeShaderModule));

		ShaderModuleCreateInfo.codeSize = LuminanceSumComputeShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = LuminanceSumComputeShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &LuminanceSumComputeShaderModule));

		ShaderModuleCreateInfo.codeSize = LuminanceAvgComputeShaderByteCodeLength;
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

		vkDestroyShaderModule(Device, LuminanceCalcComputeShaderModule, nullptr);
		vkDestroyShaderModule(Device, LuminanceSumComputeShaderModule, nullptr);
		vkDestroyShaderModule(Device, LuminanceAvgComputeShaderModule, nullptr);
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

		for (int i = 0; i < 7; i++)
		{
			ImageCreateInfo.extent.height = ResolutionHeight >> i;
			ImageCreateInfo.extent.width = ResolutionWidth >> i;

			SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[0][i]));
			SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[1][i]));
			SAFE_VK(vkCreateImage(Device, &ImageCreateInfo, nullptr, &BloomTextures[2][i]));
		}

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkDeviceSize BloomTexturesOffsets[3][7];

		for (int i = 0; i < 7; i++)
		{
			VkMemoryRequirements MemoryRequirements;

			vkGetImageMemoryRequirements(Device, BloomTextures[0][i], &MemoryRequirements);

			BloomTexturesOffsets[0][i] = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
			MemoryAllocateInfo.allocationSize = BloomTexturesOffsets[0][i] + MemoryRequirements.size;

			vkGetImageMemoryRequirements(Device, BloomTextures[1][i], &MemoryRequirements);

			BloomTexturesOffsets[1][i] = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
			MemoryAllocateInfo.allocationSize = BloomTexturesOffsets[1][i] + MemoryRequirements.size;

			vkGetImageMemoryRequirements(Device, BloomTextures[2][i], &MemoryRequirements);

			BloomTexturesOffsets[2][i] = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
			MemoryAllocateInfo.allocationSize = BloomTexturesOffsets[2][i] + MemoryRequirements.size;
		}

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory10));

		for (int i = 0; i < 7; i++)
		{
			SAFE_VK(vkBindImageMemory(Device, BloomTextures[0][i], GPUMemory10, BloomTexturesOffsets[0][i]));
			SAFE_VK(vkBindImageMemory(Device, BloomTextures[1][i], GPUMemory10, BloomTexturesOffsets[1][i]));
			SAFE_VK(vkBindImageMemory(Device, BloomTextures[2][i], GPUMemory10, BloomTexturesOffsets[2][i]));
		}

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

		VkFramebufferCreateInfo FramebufferCreateInfo;
		FramebufferCreateInfo.attachmentCount = 1;
		FramebufferCreateInfo.flags = 0;
		FramebufferCreateInfo.layers = 1;
		FramebufferCreateInfo.pNext = nullptr;
		FramebufferCreateInfo.renderPass = BloomRenderPass;
		FramebufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

		for (int i = 0; i < 7; i++)
		{
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

		SIZE_T BrightPassPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.BrightPass");
		ScopedMemoryBlockArray<BYTE> BrightPassPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(BrightPassPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.BrightPass", BrightPassPixelShaderByteCodeData);

		SIZE_T ImageResamplePixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.ImageResample");
		ScopedMemoryBlockArray<BYTE> ImageResamplePixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(ImageResamplePixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.ImageResample", ImageResamplePixelShaderByteCodeData);

		SIZE_T HorizontalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.HorizontalBlur");
		ScopedMemoryBlockArray<BYTE> HorizontalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HorizontalBlurPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.HorizontalBlur", HorizontalBlurPixelShaderByteCodeData);

		SIZE_T VerticalBlurPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.VerticalBlur");
		ScopedMemoryBlockArray<BYTE> VerticalBlurPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(VerticalBlurPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.VerticalBlur", VerticalBlurPixelShaderByteCodeData);

		VkShaderModule BrightPassPixelShaderModule, ImageResamplePixelShaderModule, HorizontalBlurPixelShaderModule, VerticalBlurPixelShaderModule;

		ShaderModuleCreateInfo.codeSize = BrightPassPixelShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = BrightPassPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &BrightPassPixelShaderModule));

		ShaderModuleCreateInfo.codeSize = ImageResamplePixelShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = ImageResamplePixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ImageResamplePixelShaderModule));

		ShaderModuleCreateInfo.codeSize = HorizontalBlurPixelShaderByteCodeLength;
		ShaderModuleCreateInfo.flags = 0;
		ShaderModuleCreateInfo.pCode = HorizontalBlurPixelShaderByteCodeData;
		ShaderModuleCreateInfo.pNext = nullptr;
		ShaderModuleCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		SAFE_VK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &HorizontalBlurPixelShaderModule));

		ShaderModuleCreateInfo.codeSize = VerticalBlurPixelShaderByteCodeLength;
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

		vkDestroyShaderModule(Device, BrightPassPixelShaderModule, nullptr);
		vkDestroyShaderModule(Device, ImageResamplePixelShaderModule, nullptr);
		vkDestroyShaderModule(Device, HorizontalBlurPixelShaderModule, nullptr);
		vkDestroyShaderModule(Device, VerticalBlurPixelShaderModule, nullptr);
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

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = 0;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		VkMemoryRequirements MemoryRequirements;

		vkGetImageMemoryRequirements(Device, ToneMappedImageTexture, &MemoryRequirements);

		VkDeviceSize ToneMappedImageTextureOffset = (MemoryAllocateInfo.allocationSize == 0) ? 0 : MemoryAllocateInfo.allocationSize + (MemoryRequirements.alignment - MemoryAllocateInfo.allocationSize % MemoryRequirements.alignment);
		MemoryAllocateInfo.allocationSize = ToneMappedImageTextureOffset + MemoryRequirements.size;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &GPUMemory11));

		SAFE_VK(vkBindImageMemory(Device, ToneMappedImageTexture, GPUMemory11, ToneMappedImageTextureOffset));

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

		SIZE_T HDRToneMappingPixelShaderByteCodeLength = Engine::GetEngine().GetFileSystem().GetFileSize("SPIRV.HDRToneMapping");
		ScopedMemoryBlockArray<BYTE> HDRToneMappingPixelShaderByteCodeData = Engine::GetEngine().GetMemoryManager().GetGlobalStack().AllocateFromStack<BYTE>(HDRToneMappingPixelShaderByteCodeLength);
		Engine::GetEngine().GetFileSystem().LoadFile("SPIRV.HDRToneMapping", HDRToneMappingPixelShaderByteCodeData);

		VkShaderModule HDRToneMappingPixelShaderModule;

		ShaderModuleCreateInfo.codeSize = HDRToneMappingPixelShaderByteCodeLength;
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

		vkDestroyShaderModule(Device, HDRToneMappingPixelShaderModule, nullptr);
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

	vkDestroyShaderModule(Device, FullScreenQuadShaderModule, nullptr);
}

void RenderDeviceVulkan::ShutdownDevice()
{
	CurrentFrameIndex = (CurrentFrameIndex + 1) % 2;

	SAFE_VK(vkWaitForFences(Device, 1, &FrameSyncFences[CurrentFrameIndex], VK_FALSE, UINT64_MAX));

	for (RenderMesh* renderMesh : RenderMeshDestructionQueue)
	//for (size_t i = 0; i < RenderMeshDestructionQueue.GetLength(); i++)
	{
		//RenderMesh* renderMesh = RenderMeshDestructionQueue[i];

		vkDestroyBuffer(Device, ((RenderMeshVulkan*)renderMesh)->MeshBuffer, nullptr);

		delete (RenderMeshVulkan*)renderMesh;
	}

	RenderMeshDestructionQueue.Clear();

	for (RenderMaterial* renderMaterial : RenderMaterialDestructionQueue)
	//for (size_t i = 0; i < RenderMeshDestructionQueue.GetLength(); i++)
	{
		//RenderMaterial* renderMaterial = RenderMaterialDestructionQueue[i];

		vkDestroyPipeline(Device, ((RenderMaterialVulkan*)renderMaterial)->GBufferOpaquePassPipeline, nullptr);
		vkDestroyPipeline(Device, ((RenderMaterialVulkan*)renderMaterial)->ShadowMapPassPipeline, nullptr);

		delete (RenderMaterialVulkan*)renderMaterial;
	}
	
	RenderMaterialDestructionQueue.Clear();

	for (RenderTexture* renderTexture : RenderTextureDestructionQueue)
	//for (size_t i = 0; i < RenderMeshDestructionQueue.GetLength(); i++)
	{
		//RenderTexture* renderTexture = RenderTextureDestructionQueue[i];

		vkDestroyImageView(Device, ((RenderTextureVulkan*)renderTexture)->TextureView, nullptr);
		vkDestroyImage(Device, ((RenderTextureVulkan*)renderTexture)->Texture, nullptr);

		delete (RenderTextureVulkan*)renderTexture;
	}

	RenderTextureDestructionQueue.Clear();

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

	vkDestroyFence(Device, FrameSyncFences[0], nullptr);
	vkDestroyFence(Device, FrameSyncFences[1], nullptr);
	vkDestroyFence(Device, CopySyncFence, nullptr);
	vkDestroySemaphore(Device, ImageAvailabilitySemaphore, nullptr);
	vkDestroySemaphore(Device, ImagePresentationSemaphore, nullptr);

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
		vkDestroyFramebuffer(Device, BackBufferFrameBuffers[i], nullptr);
	}

	vkDestroyRenderPass(Device, BackBufferResolveRenderPass, nullptr);

#ifdef _DEBUG
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");

	vkDestroyDebugUtilsMessengerEXT(Instance, DebugUtilsMessenger, nullptr);
#endif

	vkDestroySampler(Device, TextureSampler, nullptr);
	vkDestroySampler(Device, ShadowMapSampler, nullptr);
	vkDestroySampler(Device, BiLinearSampler, nullptr);
	vkDestroySampler(Device, MinSampler, nullptr);

	vkDestroyImageView(Device, GBufferTexturesViews[0], nullptr);
	vkDestroyImageView(Device, GBufferTexturesViews[1], nullptr);

	vkDestroyImage(Device, GBufferTextures[0], nullptr);
	vkDestroyImage(Device, GBufferTextures[1], nullptr);

	vkDestroyImageView(Device, DepthBufferTextureView, nullptr);
	vkDestroyImageView(Device, DepthBufferTextureDepthReadView, nullptr);

	vkDestroyImage(Device, DepthBufferTexture, nullptr);

	vkDestroyRenderPass(Device, GBufferClearRenderPass, nullptr);
	vkDestroyRenderPass(Device, GBufferDrawRenderPass, nullptr);

	vkDestroyFramebuffer(Device, GBufferFrameBuffer, nullptr);

	vkDestroyBuffer(Device, GPUConstantBuffer, nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers[1], nullptr);

	vkFreeMemory(Device, GPUMemory1, nullptr);
	vkFreeMemory(Device, CPUMemory1, nullptr);

	// ===============================================================================================================

	vkDestroyImageView(Device, ResolvedDepthBufferTextureView, nullptr);
	vkDestroyImageView(Device, ResolvedDepthBufferTextureDepthOnlyView, nullptr);

	vkDestroyImage(Device, ResolvedDepthBufferTexture, nullptr);

	vkFreeMemory(Device, GPUMemory2, nullptr);

	vkDestroyRenderPass(Device, MSAADepthBufferResolveRenderPass, nullptr);

	vkDestroyFramebuffer(Device, ResolvedDepthFrameBuffer, nullptr);

	// ===============================================================================================================

	vkDestroyImage(Device, OcclusionBufferTexture, nullptr);
	vkDestroyImageView(Device, OcclusionBufferTextureView, nullptr);
	vkDestroyBuffer(Device, OcclusionBufferReadbackBuffers[0], nullptr);
	vkDestroyBuffer(Device, OcclusionBufferReadbackBuffers[1], nullptr);

	vkFreeMemory(Device, GPUMemory3, nullptr);
	vkFreeMemory(Device, CPUMemory3, nullptr);

	vkDestroyRenderPass(Device, OcclusionBufferRenderPass, nullptr);
	vkDestroyFramebuffer(Device, OcclusionBufferFrameBuffer, nullptr);

	vkDestroyPipeline(Device, OcclusionBufferPipeline, nullptr);
	vkDestroyPipelineLayout(Device, OcclusionBufferPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, OcclusionBufferSetLayout, nullptr);

	// ===============================================================================================================

	vkDestroyImage(Device, CascadedShadowMapTextures[0], nullptr);
	vkDestroyImage(Device, CascadedShadowMapTextures[1], nullptr);
	vkDestroyImage(Device, CascadedShadowMapTextures[2], nullptr);
	vkDestroyImage(Device, CascadedShadowMapTextures[3], nullptr);

	vkDestroyImageView(Device, CascadedShadowMapTexturesViews[0], nullptr);
	vkDestroyImageView(Device, CascadedShadowMapTexturesViews[1], nullptr);
	vkDestroyImageView(Device, CascadedShadowMapTexturesViews[2], nullptr);
	vkDestroyImageView(Device, CascadedShadowMapTexturesViews[3], nullptr);

	vkDestroyRenderPass(Device, ShadowMapClearRenderPass, nullptr);
	vkDestroyRenderPass(Device, ShadowMapDrawRenderPass, nullptr);

	vkDestroyFramebuffer(Device, CascadedShadowMapFrameBuffers[0], nullptr);
	vkDestroyFramebuffer(Device, CascadedShadowMapFrameBuffers[1], nullptr);
	vkDestroyFramebuffer(Device, CascadedShadowMapFrameBuffers[2], nullptr);
	vkDestroyFramebuffer(Device, CascadedShadowMapFrameBuffers[3], nullptr);

	vkDestroyBuffer(Device, GPUConstantBuffers2[0], nullptr);
	vkDestroyBuffer(Device, GPUConstantBuffers2[1], nullptr);
	vkDestroyBuffer(Device, GPUConstantBuffers2[2], nullptr);
	vkDestroyBuffer(Device, GPUConstantBuffers2[3], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[0][0], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[0][1], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[1][0], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[1][1], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[2][0], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[2][1], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[3][0], nullptr);
	vkDestroyBuffer(Device, CPUConstantBuffers2[3][1], nullptr);

	vkFreeMemory(Device, GPUMemory4, nullptr);
	vkFreeMemory(Device, CPUMemory4, nullptr);

	// ===============================================================================================================

	vkDestroyImage(Device, ShadowMaskTexture, nullptr);
	vkDestroyImageView(Device, ShadowMaskTextureView, nullptr);

	vkDestroyRenderPass(Device, ShadowMaskRenderPass, nullptr);
	vkDestroyFramebuffer(Device, ShadowMaskFrameBuffer, nullptr);

	vkDestroyBuffer(Device, GPUShadowResolveConstantBuffer, nullptr);
	vkDestroyBuffer(Device, CPUShadowResolveConstantBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPUShadowResolveConstantBuffers[1], nullptr);

	vkFreeMemory(Device, GPUMemory5, nullptr);
	vkFreeMemory(Device, CPUMemory5, nullptr);

	vkDestroyPipeline(Device, ShadowResolvePipeline, nullptr);
	vkDestroyPipelineLayout(Device, ShadowResolvePipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, ShadowResolveSetLayout, nullptr);

	// ===============================================================================================================

	vkDestroyImage(Device, HDRSceneColorTexture, nullptr);
	vkDestroyImageView(Device, HDRSceneColorTextureView, nullptr);

	vkDestroyRenderPass(Device, DeferredLightingRenderPass, nullptr);
	vkDestroyFramebuffer(Device, HDRSceneColorFrameBuffer, nullptr);

	vkDestroyBuffer(Device, GPUDeferredLightingConstantBuffer, nullptr);
	vkDestroyBuffer(Device, CPUDeferredLightingConstantBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPUDeferredLightingConstantBuffers[1], nullptr);

	vkDestroyBufferView(Device, LightClustersBufferView, nullptr);
	vkDestroyBuffer(Device, GPULightClustersBuffer, nullptr);
	vkDestroyBuffer(Device, CPULightClustersBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPULightClustersBuffers[1], nullptr);

	vkDestroyBufferView(Device, LightIndicesBufferView, nullptr);
	vkDestroyBuffer(Device, GPULightIndicesBuffer, nullptr);
	vkDestroyBuffer(Device, CPULightIndicesBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPULightIndicesBuffers[1], nullptr);

	vkDestroyBuffer(Device, GPUPointLightsBuffer, nullptr);
	vkDestroyBuffer(Device, CPUPointLightsBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPUPointLightsBuffers[1], nullptr);

	vkFreeMemory(Device, GPUMemory6, nullptr);
	vkFreeMemory(Device, CPUMemory6, nullptr);

	vkDestroyPipeline(Device, DeferredLightingPipeline, nullptr);
	vkDestroyPipelineLayout(Device, DeferredLightingPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, DeferredLightingSetLayout, nullptr);

	// ===============================================================================================================

	vkDestroyPipelineLayout(Device, SkyAndSunPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, SkyAndSunSetLayout, nullptr);

	vkDestroyRenderPass(Device, SkyAndSunRenderPass, nullptr);
	vkDestroyFramebuffer(Device, HDRSceneColorAndDepthFrameBuffer, nullptr);

	vkDestroyBuffer(Device, SkyVertexBuffer, nullptr);
	vkDestroyBuffer(Device, SkyIndexBuffer, nullptr);
	vkDestroyBuffer(Device, GPUSkyConstantBuffer, nullptr);
	vkDestroyBuffer(Device, CPUSkyConstantBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPUSkyConstantBuffers[1], nullptr);
	vkDestroyPipeline(Device, SkyPipeline, nullptr);
	vkDestroyImage(Device, SkyTexture, nullptr);
	vkDestroyImageView(Device, SkyTextureView, nullptr);

	vkDestroyBuffer(Device, SunVertexBuffer, nullptr);
	vkDestroyBuffer(Device, SunIndexBuffer, nullptr);
	vkDestroyBuffer(Device, GPUSunConstantBuffer, nullptr);
	vkDestroyBuffer(Device, CPUSunConstantBuffers[0], nullptr);
	vkDestroyBuffer(Device, CPUSunConstantBuffers[1], nullptr);
	vkDestroyPipeline(Device, SunPipeline, nullptr);
	vkDestroyImage(Device, SunTexture, nullptr);
	vkDestroyImageView(Device, SunTextureView, nullptr);

	vkFreeMemory(Device, GPUMemory7, nullptr);
	vkFreeMemory(Device, CPUMemory7, nullptr);

	vkDestroyPipeline(Device, FogPipeline, nullptr);
	vkDestroyPipelineLayout(Device, FogPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, FogSetLayout, nullptr);

	// ===============================================================================================================

	vkDestroyImage(Device, ResolvedHDRSceneColorTexture, nullptr);
	vkDestroyImageView(Device, ResolvedHDRSceneColorTextureView, nullptr);
	vkFreeMemory(Device, GPUMemory8, nullptr);

	vkDestroyRenderPass(Device, HDRSceneColorResolveRenderPass, nullptr);
	vkDestroyFramebuffer(Device, HDRSceneColorResolveFrameBuffer, nullptr);

	// ===============================================================================================================

	vkDestroyImage(Device, SceneLuminanceTextures[0], nullptr);
	vkDestroyImage(Device, SceneLuminanceTextures[1], nullptr);
	vkDestroyImage(Device, SceneLuminanceTextures[2], nullptr);
	vkDestroyImage(Device, SceneLuminanceTextures[3], nullptr);
	vkDestroyImageView(Device, SceneLuminanceTexturesViews[0], nullptr);
	vkDestroyImageView(Device, SceneLuminanceTexturesViews[1], nullptr);
	vkDestroyImageView(Device, SceneLuminanceTexturesViews[2], nullptr);
	vkDestroyImageView(Device, SceneLuminanceTexturesViews[3], nullptr);

	vkDestroyImage(Device, AverageLuminanceTexture, nullptr);
	vkDestroyImageView(Device, AverageLuminanceTextureView, nullptr);

	vkFreeMemory(Device, GPUMemory9, nullptr);

	vkDestroyPipelineLayout(Device, LuminancePassPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, LuminancePassSetLayout, nullptr);

	vkDestroyPipeline(Device, LuminanceCalcPipeline, nullptr);
	vkDestroyPipeline(Device, LuminanceSumPipeline, nullptr);
	vkDestroyPipeline(Device, LuminanceAvgPipeline, nullptr);

	// ===============================================================================================================

	for (int i = 0; i < 7; i++)
	{
		vkDestroyFramebuffer(Device, BloomTexturesFrameBuffers[0][i], nullptr);
		vkDestroyFramebuffer(Device, BloomTexturesFrameBuffers[1][i], nullptr);
		vkDestroyFramebuffer(Device, BloomTexturesFrameBuffers[2][i], nullptr);

		vkDestroyImageView(Device, BloomTexturesViews[0][i], nullptr);
		vkDestroyImageView(Device, BloomTexturesViews[1][i], nullptr);
		vkDestroyImageView(Device, BloomTexturesViews[2][i], nullptr);

		vkDestroyImage(Device, BloomTextures[0][i], nullptr);
		vkDestroyImage(Device, BloomTextures[1][i], nullptr);
		vkDestroyImage(Device, BloomTextures[2][i], nullptr);
	}

	vkFreeMemory(Device, GPUMemory10, nullptr);

	vkDestroyRenderPass(Device, BloomRenderPass, nullptr);

	vkDestroyPipelineLayout(Device, BloomPassPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, BloomPassSetLayout, nullptr);

	vkDestroyPipeline(Device, BrightPassPipeline, nullptr);
	vkDestroyPipeline(Device, DownSamplePipeline, nullptr);
	vkDestroyPipeline(Device, HorizontalBlurPipeline, nullptr);
	vkDestroyPipeline(Device, VerticalBlurPipeline, nullptr);
	vkDestroyPipeline(Device, UpSampleWithAddBlendPipeline, nullptr);

	// ===============================================================================================================

	vkDestroyImageView(Device, ToneMappedImageTextureView, nullptr);
	vkDestroyImage(Device, ToneMappedImageTexture, nullptr);
	vkFreeMemory(Device, GPUMemory11, nullptr);

	vkDestroyRenderPass(Device, HDRToneMappingRenderPass, nullptr);
	vkDestroyFramebuffer(Device, ToneMappedImageFrameBuffer, nullptr);

	vkDestroyPipelineLayout(Device, HDRToneMappingPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, HDRToneMappingSetLayout, nullptr);

	vkDestroyPipeline(Device, HDRToneMappingPipeline, nullptr);

	vkDestroyDevice(Device, nullptr);
	vkDestroyInstance(Instance, nullptr);
}

void RenderDeviceVulkan::TickDevice(float DeltaTime)
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

	DynamicArray<StaticMeshComponent*> AllStaticMeshComponents = renderScene.GetStaticMeshComponents();
	DynamicArray<StaticMeshComponent*> VisbleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ViewProjMatrix, true);
	size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.GetLength();

	DynamicArray<PointLightComponent*> AllPointLightComponents = renderScene.GetPointLightComponents();
	DynamicArray<PointLightComponent*> VisblePointLightComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisiblePointLightsInFrustum(AllPointLightComponents, ViewProjMatrix);

	Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().ClusterizeLights(VisblePointLightComponents, ViewMatrix);

	DynamicArray<PointLight> PointLights;

	for (PointLightComponent *pointLightComponent : VisblePointLightComponents)
	//for (size_t i = 0; i < VisblePointLightComponents.GetLength(); i++)
	{
		//PointLightComponent *pointLightComponent = VisblePointLightComponents[i];

		PointLight pointLight;
		pointLight.Brightness = pointLightComponent->GetBrightness();
		pointLight.Color = pointLightComponent->GetColor();
		pointLight.Position = pointLightComponent->GetTransformComponent()->GetLocation();
		pointLight.Radius = pointLightComponent->GetRadius();

		PointLights.Add(pointLight);
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

		SAFE_VK(vkMapMemory(Device, CPUMemory1, ConstantBuffersOffets1[CurrentFrameIndex], VK_WHOLE_SIZE, 0, &ConstantBufferData));

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

		vkUnmapMemory(Device, CPUMemory1);

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

		for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
		{
			StaticMeshComponent *staticMeshComponent = VisbleStaticMeshComponents[k];

			RenderMeshVulkan *renderMesh = (RenderMeshVulkan*)staticMeshComponent->GetStaticMesh()->GetRenderMesh();
			RenderMaterialVulkan *renderMaterial = (RenderMaterialVulkan*)staticMeshComponent->GetMaterial()->GetRenderMaterial();
			RenderTextureVulkan *renderTexture0 = (RenderTextureVulkan*)staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
			RenderTextureVulkan *renderTexture1 = (RenderTextureVulkan*)staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

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

			VkBuffer VertexBuffers[3] =
			{
				renderMesh->MeshBuffer,
				renderMesh->MeshBuffer,
				renderMesh->MeshBuffer
			};

			VkDeviceSize VertexBuffersOffsets[3] =
			{
				0,
				9 * 9 * 6 * sizeof(XMFLOAT3),
				9 * 9 * 6 * (sizeof(XMFLOAT3) + sizeof(XMFLOAT2))
			};

			vkCmdBindVertexBuffers(CommandBuffers[CurrentFrameIndex], 0, 3, VertexBuffers, VertexBuffersOffsets);
			vkCmdBindIndexBuffer(CommandBuffers[CurrentFrameIndex], renderMesh->MeshBuffer, 9 * 9 * 6 * (4 * sizeof(XMFLOAT3) + sizeof(XMFLOAT2)), VkIndexType::VK_INDEX_TYPE_UINT16);

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

		SAFE_VK(vkMapMemory(Device, CPUMemory3, OcclusionBuffersOffsets[CurrentFrameIndex], VK_WHOLE_SIZE, 0, (void**)&MappedData));

		for (int i = 0; i < 144; i++)
		{
			memcpy(Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetOcclusionBufferData() + i * 256, MappedData + i * 256, 256 * 4);
		}

		vkUnmapMemory(Device, CPUMemory3);
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
			DynamicArray<StaticMeshComponent*> AllStaticMeshComponents = Engine::GetEngine().GetGameFramework().GetWorld().GetRenderScene().GetStaticMeshComponents();
			DynamicArray<StaticMeshComponent*> VisbleStaticMeshComponents = Engine::GetEngine().GetRenderSystem().GetCullingSubSystem().GetVisibleStaticMeshesInFrustum(AllStaticMeshComponents, ShadowViewProjMatrices[i], false);
			size_t VisbleStaticMeshComponentsCount = VisbleStaticMeshComponents.GetLength();

			void *ConstantBufferData;
			size_t ConstantBufferOffset = 0;

			SAFE_VK(vkMapMemory(Device, CPUMemory4, ConstantBuffersOffets2[i][CurrentFrameIndex], VK_WHOLE_SIZE, 0, &ConstantBufferData));

			for (int k = 0; k < VisbleStaticMeshComponentsCount; k++)
			{
				XMMATRIX WorldMatrix = VisbleStaticMeshComponents[k]->GetTransformComponent()->GetTransformMatrix();
				XMMATRIX WVPMatrix = WorldMatrix * ShadowViewProjMatrices[i];

				memcpy((BYTE*)ConstantBufferData + ConstantBufferOffset, &WVPMatrix, sizeof(XMMATRIX));

				ConstantBufferOffset += 256;
			}

			vkUnmapMemory(Device, CPUMemory4);

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

				RenderMeshVulkan *renderMesh = (RenderMeshVulkan*)staticMeshComponent->GetStaticMesh()->GetRenderMesh();
				RenderMaterialVulkan *renderMaterial = (RenderMaterialVulkan*)staticMeshComponent->GetMaterial()->GetRenderMaterial();
				RenderTextureVulkan *renderTexture0 = (RenderTextureVulkan*)staticMeshComponent->GetMaterial()->GetTexture(0)->GetRenderTexture();
				RenderTextureVulkan *renderTexture1 = (RenderTextureVulkan*)staticMeshComponent->GetMaterial()->GetTexture(1)->GetRenderTexture();

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

				vkCmdBindVertexBuffers(CommandBuffers[CurrentFrameIndex], 0, 1, &renderMesh->MeshBuffer, &Offset);
				vkCmdBindIndexBuffer(CommandBuffers[CurrentFrameIndex], renderMesh->MeshBuffer, 9 * 9 * 6 * (4 * sizeof(XMFLOAT3) + sizeof(XMFLOAT2)), VkIndexType::VK_INDEX_TYPE_UINT16);

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

		SAFE_VK(vkMapMemory(Device, CPUMemory5, ConstantBuffersOffets3[CurrentFrameIndex], VK_WHOLE_SIZE, 0, &ConstantBufferData));

		ShadowResolveConstantBuffer& ConstantBuffer = *((ShadowResolveConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.ReProjMatrices[0] = ReProjMatrices[0];
		ConstantBuffer.ReProjMatrices[1] = ReProjMatrices[1];
		ConstantBuffer.ReProjMatrices[2] = ReProjMatrices[2];
		ConstantBuffer.ReProjMatrices[3] = ReProjMatrices[3];

		ConstantBufferOffset += 256;

		vkUnmapMemory(Device, CPUMemory5);

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

		SAFE_VK(vkMapMemory(Device, CPUMemory6, ConstantBuffersOffets4[CurrentFrameIndex], VK_WHOLE_SIZE, 0, &ConstantBufferData));

		XMMATRIX InvViewProjMatrix = XMMatrixInverse(nullptr, ViewProjMatrix);

		DeferredLightingConstantBuffer& ConstantBuffer = *((DeferredLightingConstantBuffer*)((BYTE*)ConstantBufferData));

		ConstantBuffer.InvViewProjMatrix = InvViewProjMatrix;
		ConstantBuffer.CameraWorldPosition = CameraLocation;

		vkUnmapMemory(Device, CPUMemory6);

		void *DynamicBufferData;

		SAFE_VK(vkMapMemory(Device, CPUMemory6, DynamicBuffersOffsets[0][CurrentFrameIndex], VK_WHOLE_SIZE, 0, &DynamicBufferData));

		memcpy(DynamicBufferData, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetLightClustersData(), 32 * 18 * 24 * 2 * sizeof(uint32_t));

		vkUnmapMemory(Device, CPUMemory6);

		SAFE_VK(vkMapMemory(Device, CPUMemory6, DynamicBuffersOffsets[1][CurrentFrameIndex], VK_WHOLE_SIZE, 0, &DynamicBufferData));

		memcpy(DynamicBufferData, Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetLightIndicesData(), Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t));

		vkUnmapMemory(Device, CPUMemory6);

		SAFE_VK(vkMapMemory(Device, CPUMemory6, DynamicBuffersOffsets[2][CurrentFrameIndex], VK_WHOLE_SIZE, 0, &DynamicBufferData));

		memcpy(DynamicBufferData, PointLights.GetData(), PointLights.GetLength() * sizeof(PointLight));

		vkUnmapMemory(Device, CPUMemory6);

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
		BufferCopy.size = Engine::GetEngine().GetRenderSystem().GetClusterizationSubSystem().GetTotalIndexCount() * sizeof(uint16_t);
		vkCmdCopyBuffer(CommandBuffers[CurrentFrameIndex], CPULightIndicesBuffers[CurrentFrameIndex], GPULightIndicesBuffer, 1, &BufferCopy);
		BufferCopy.size = PointLights.GetLength() * sizeof(PointLight);
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

		SAFE_VK(vkMapMemory(Device, CPUMemory7, ConstantBuffersOffets5[0][CurrentFrameIndex], VK_WHOLE_SIZE, 0, &ConstantBufferData));

		SkyConstantBuffer& skyConstantBuffer = *((SkyConstantBuffer*)((BYTE*)ConstantBufferData));

		skyConstantBuffer.WVPMatrix = SkyWVPMatrix;

		vkUnmapMemory(Device, CPUMemory7);

		XMFLOAT3 SunPosition(-500.0f + CameraLocation.x, 500.0f + CameraLocation.y, -500.f + CameraLocation.z);

		SAFE_VK(vkMapMemory(Device, CPUMemory7, ConstantBuffersOffets5[1][CurrentFrameIndex], VK_WHOLE_SIZE, 0, &ConstantBufferData));

		SunConstantBuffer& sunConstantBuffer = *((SunConstantBuffer*)((BYTE*)ConstantBufferData));

		sunConstantBuffer.ViewMatrix = ViewMatrix;
		sunConstantBuffer.ProjMatrix = ProjMatrix;
		sunConstantBuffer.SunPosition = SunPosition;

		vkUnmapMemory(Device, CPUMemory7);

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

RenderMesh* RenderDeviceVulkan::CreateRenderMesh(const RenderMeshCreateInfo& renderMeshCreateInfo)
{
	RenderMeshVulkan *renderMesh = new RenderMeshVulkan();

	VkBufferCreateInfo BufferCreateInfo;
	BufferCreateInfo.flags = 0;
	BufferCreateInfo.pNext = nullptr;
	BufferCreateInfo.pQueueFamilyIndices = nullptr;
	BufferCreateInfo.queueFamilyIndexCount = 0;
	BufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
	BufferCreateInfo.size = sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount;
	BufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.usage = VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	SAFE_VK(vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &renderMesh->MeshBuffer));

	VkMemoryRequirements MemoryRequirements;

	vkGetBufferMemoryRequirements(Device, renderMesh->MeshBuffer, &MemoryRequirements);

	size_t AlignedResourceOffset = BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] + (MemoryRequirements.alignment - BufferMemoryHeapOffsets[CurrentBufferMemoryHeapIndex] % MemoryRequirements.alignment);

	if (AlignedResourceOffset + MemoryRequirements.size > BUFFER_MEMORY_HEAP_SIZE)
	{
		++CurrentBufferMemoryHeapIndex;

		VkMemoryAllocateInfo MemoryAllocateInfo;
		MemoryAllocateInfo.allocationSize = BUFFER_MEMORY_HEAP_SIZE;
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
		MemoryAllocateInfo.pNext = nullptr;
		MemoryAllocateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		SAFE_VK(vkAllocateMemory(Device, &MemoryAllocateInfo, nullptr, &BufferMemoryHeaps[CurrentBufferMemoryHeapIndex]));

		AlignedResourceOffset = 0;
	}

	SAFE_VK(vkBindBufferMemory(Device, renderMesh->MeshBuffer, BufferMemoryHeaps[CurrentBufferMemoryHeapIndex], AlignedResourceOffset));

	void *MappedData;

	SAFE_VK(vkMapMemory(Device, UploadHeap, 0, VK_WHOLE_SIZE, 0, &MappedData));
	memcpy((BYTE*)MappedData, renderMeshCreateInfo.MeshData, sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount);
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

	vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_HOST_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

	VkBufferCopy BufferCopy;
	BufferCopy.dstOffset = 0;
	BufferCopy.size = sizeof(Vertex) * renderMeshCreateInfo.VertexCount + sizeof(WORD) * renderMeshCreateInfo.IndexCount;
	BufferCopy.srcOffset = 0;

	vkCmdCopyBuffer(CommandBuffers[0], UploadBuffer, renderMesh->MeshBuffer, 1, &BufferCopy);

	BufferMemoryBarrier.buffer = renderMesh->MeshBuffer;
	BufferMemoryBarrier.dstAccessMask = VkAccessFlagBits::VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VkAccessFlagBits::VK_ACCESS_INDEX_READ_BIT;
	BufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.offset = 0;
	BufferMemoryBarrier.pNext = nullptr;
	BufferMemoryBarrier.size = VK_WHOLE_SIZE;
	BufferMemoryBarrier.srcAccessMask = VkAccessFlagBits::VK_ACCESS_TRANSFER_WRITE_BIT;
	BufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	BufferMemoryBarrier.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	
	vkCmdPipelineBarrier(CommandBuffers[0], VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &BufferMemoryBarrier, 0, nullptr);

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

RenderTexture* RenderDeviceVulkan::CreateRenderTexture(const RenderTextureCreateInfo& renderTextureCreateInfo)
{
	RenderTextureVulkan *renderTexture = new RenderTextureVulkan();

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
		MemoryAllocateInfo.memoryTypeIndex = DefaultMemoryHeapIndex;
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

RenderMaterial* RenderDeviceVulkan::CreateRenderMaterial(const RenderMaterialCreateInfo& renderMaterialCreateInfo)
{
	RenderMaterialVulkan *renderMaterial = new RenderMaterialVulkan();

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

	VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo;
	ZeroMemory(&PipelineMultisampleStateCreateInfo, sizeof(VkPipelineMultisampleStateCreateInfo));
	PipelineMultisampleStateCreateInfo.pNext = nullptr;
	PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_8_BIT;
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
	VertexInputAttributeDescriptions[1].binding = 1;
	VertexInputAttributeDescriptions[1].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
	VertexInputAttributeDescriptions[1].location = 1;
	VertexInputAttributeDescriptions[1].offset = 0;
	VertexInputAttributeDescriptions[2].binding = 2;
	VertexInputAttributeDescriptions[2].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[2].location = 2;
	VertexInputAttributeDescriptions[2].offset = 0;
	VertexInputAttributeDescriptions[3].binding = 2;
	VertexInputAttributeDescriptions[3].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[3].location = 3;
	VertexInputAttributeDescriptions[3].offset = 12;
	VertexInputAttributeDescriptions[4].binding = 2;
	VertexInputAttributeDescriptions[4].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	VertexInputAttributeDescriptions[4].location = 4;
	VertexInputAttributeDescriptions[4].offset = 24;

	VkVertexInputBindingDescription VertexInputBindingDescriptions[3];
	VertexInputBindingDescriptions[0].binding = 0;
	VertexInputBindingDescriptions[0].inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
	VertexInputBindingDescriptions[0].stride = 12;
	VertexInputBindingDescriptions[1].binding = 1;
	VertexInputBindingDescriptions[1].inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
	VertexInputBindingDescriptions[1].stride = 8;
	VertexInputBindingDescriptions[2].binding = 2;
	VertexInputBindingDescriptions[2].inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
	VertexInputBindingDescriptions[2].stride = 36;

	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo;
	PipelineVertexInputStateCreateInfo.flags = 0;
	PipelineVertexInputStateCreateInfo.pNext = nullptr;
	PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttributeDescriptions;
	PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = VertexInputBindingDescriptions;
	PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 5;
	PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 3;

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

	ZeroMemory(&PipelineMultisampleStateCreateInfo, sizeof(VkPipelineMultisampleStateCreateInfo));
	PipelineMultisampleStateCreateInfo.pNext = nullptr;
	PipelineMultisampleStateCreateInfo.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
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

	PipelineVertexInputStateCreateInfo.flags = 0;
	PipelineVertexInputStateCreateInfo.pNext = nullptr;
	PipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = VertexInputAttributeDescriptions;
	PipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = VertexInputBindingDescriptions;
	PipelineVertexInputStateCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	PipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
	PipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;

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
	vkDestroyShaderModule(Device, ShadowMapPassVertexShaderModule, nullptr);

	return renderMaterial;
}

void RenderDeviceVulkan::DestroyRenderMesh(RenderMesh* renderMesh)
{
	RenderMeshDestructionQueue.Add(renderMesh);
}

void RenderDeviceVulkan::DestroyRenderTexture(RenderTexture* renderTexture)
{
	RenderTextureDestructionQueue.Add(renderTexture);
}

void RenderDeviceVulkan::DestroyRenderMaterial(RenderMaterial* renderMaterial)
{
	RenderMaterialDestructionQueue.Add(renderMaterial);
}

inline void RenderDeviceVulkan::CheckVulkanCallResult(VkResult Result, const char16_t* Function)
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

inline const char16_t* RenderDeviceVulkan::GetVulkanErrorMessageFromVkResult(VkResult Result)
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