#pragma once

#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <C:/VulkanSDK/1.2.162.0/Include/vulkan/vulkan.h>
#include <C:/VulkanSDK/1.2.162.0/Include/shaderc/shaderc.h>

#include <DirectXMath.h>

#pragma comment(lib, "C:/VulkanSDK/1.2.162.0/Lib/vulkan-1.lib")
#pragma comment(lib, "C:/VulkanSDK/1.2.162.0/Lib/shaderc_shared.lib")

using namespace std;
using namespace DirectX;