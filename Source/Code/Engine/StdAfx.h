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

#include <d3d12.h>
#include <dxgi1_6.h>

#include "F:/DirectXMath/Inc/DirectXMath.h"
#include "F:/DirectXMath/Inc/DirectXPackedVector.h"

#define USE_OPTICK 1
#include <F:/Optick/include/optick.h>

#pragma comment(lib, "DbgHelp.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#pragma comment(lib, "F:/Optick/lib/x64/debug/OptickCore.lib")
#else
#pragma comment(lib, "F:/Optick/lib/x64/release/OptickCore.lib")
#endif

using namespace std;
using namespace DirectX;

#define WITH_EDITOR 1