#pragma once

#ifdef PLATFORM_PC_WINDOW
#include "PCWindows/PCWindowsPlatformMemoryAllocator.h"
using PlatformMemoryAllocator = PCWindowsPlatformMemoryAllocator;
#endif