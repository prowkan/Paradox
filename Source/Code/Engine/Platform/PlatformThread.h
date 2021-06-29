#pragma once

#ifdef PLATFORM_PC_WINDOW
#include "PCWindows/PCWindowsPlatformThread.h"
using PlatformThread = PCWindowsPlatformThread;
#endif