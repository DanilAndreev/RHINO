#pragma once

#define NOMINMAX

#include "RHINO.h"
#include "RHINOTypes.h"

#include <cstdlib>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cassert>

#include <iostream>

#include "Utils/PlatformBase.h"
#include "Utils/Common.h"
#include "RHINOTypesImpl.h"


#ifdef ENABLE_API_D3D12
#include <d3d12.h>
#endif // ENABLE_API_D3D12

#ifdef ENABLE_API_VULKAN
//#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#endif // ENABLE_API_VULKAN