#pragma once

#ifdef RHINO_WIN32_SURFACE
#include <Windows.h>
struct Win32SurfaceDesc {
    HINSTANCE hInstance;
    HWND hWnd;
};
#endif

#ifdef RHINO_APPLE_SURFACE
#import <QuartzCore/CAMetalLayer.h>
struct AppleSurfaceDesc {
    CAMetalLayer* layer;
};
#endif
