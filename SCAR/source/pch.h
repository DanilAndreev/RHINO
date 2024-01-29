#pragma once

#include <SCARVersion.h>
#include <SCAR.h>

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <memory>
#include <cassert>

#if WIN32
#define SCAR_FORCEINLINE __forceinline
#else
#define SCAR_FORCEINLINE inline
#endif