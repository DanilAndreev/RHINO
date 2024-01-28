#pragma once

#ifdef __clang__
#define RHINO_PLATFORM_APPLE
#elif WIN32
#define RHINO_PLATFORM_WINDOWS
#else
#error "Unsupported platform/compiler"
#endif
