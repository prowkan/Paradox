#pragma once

#include <stdio.h>

#include <iostream>
#include <fstream>
//#include <string>
//#include <vector>
//#include <map>
//#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>

#include <Windows.h>
#include <DbgHelp.h>

#include <D:/zLib/zlib.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "D:/DirectXMath/Inc/DirectXMath.h"
#include "D:/DirectXMath/Inc/DirectXPackedVector.h"

#define USE_OPTICK 1
#include <D:/Optick/include/optick.h>

#pragma comment(lib, "DbgHelp.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#pragma comment(lib, "D:/Optick/lib/x64/debug/OptickCore.lib")
#pragma comment(lib, "D:/zLib/contrib/vstudio/vc14/x64/ZlibStatDebug/zlibstat.lib")
#else
#pragma comment(lib, "D:/Optick/lib/x64/release/OptickCore.lib")
#pragma comment(lib, "D:/zLib/contrib/vstudio/vc14/x64/ZlibStatRelease/zlibstat.lib")
#endif

using namespace std;
using namespace DirectX;

#define PLATFORM_PC_WINDOW

#define WITH_EDITOR 1