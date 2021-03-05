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

#include <d3d11_1.h>
#include <dxgi1_6.h>

#include <d3dcompiler.h>

#include <DirectXMath.h>

#define USE_OPTICK 1
#include <F:/Optick/include/optick.h>

#pragma comment(lib, "DbgHelp.lib")

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "F:/Optick/lib/x64/debug/OptickCore.lib")
#else
#pragma comment(lib, "F:/Optick/lib/x64/release/OptickCore.lib")
#endif

using namespace std;
using namespace DirectX;