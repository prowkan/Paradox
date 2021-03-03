#pragma once

#include <stdio.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <atomic>
#include <mutex>

#include <Windows.h>
#include <DbgHelp.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <C:/VulkanSDK/1.2.162.1/Include/vulkan/vulkan.h>

#include <DirectXMath.h>

#define USE_OPTICK 1
#include <F:/Optick/include/optick.h>

#pragma comment(lib, "DbgHelp.lib")

#pragma comment(lib, "C:/VulkanSDK/1.2.162.1/Lib/vulkan-1.lib")


#ifdef _DEBUG
#pragma comment(lib, "F:/Optick/lib/x64/debug/OptickCore.lib")
#else
#pragma comment(lib, "F:/Optick/lib/x64/release/OptickCore.lib")
#endif

using namespace std;
using namespace DirectX;