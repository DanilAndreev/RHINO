#pragma once

#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#define RHINO_WIN32_SURFACE
#endif // WIN32

#ifdef __clang__
#define RHINO_APPLE_SURFACE
#endif

#include "RHINO.h"
#include "RHINOTypes.h"

#include <cstdlib>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cassert>
#include <list>

#include <chrono>
#include <thread>

#include <iostream>

#include "Utils/PlatformBase.h"
#include "Utils/Common.h"


#ifdef ENABLE_API_D3D12
#include <d3d12.h>
#endif // ENABLE_API_D3D12

#ifdef ENABLE_API_VULKAN
#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif // WIN32
//#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#endif // ENABLE_API_VULKAN
