#pragma once

#include <SCARVersion.h>
#include <SCAR.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <set>
#include <cstdlib>
#include <memory>
#include <cassert>
#include <fstream>

#if WIN32
#define SCAR_FORCEINLINE __forceinline
#else
#define SCAR_FORCEINLINE inline
#endif