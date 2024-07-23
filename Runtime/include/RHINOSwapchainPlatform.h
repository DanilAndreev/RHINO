#pragma once

#ifdef RHINO_WIN32_SURFACE
#include <Windows.h>
struct RHINOWin32SurfaceDesc {
    HINSTANCE hInstance;
    HWND hWnd;
};
#endif

#ifdef RHINO_APPLE_SURFACE
#import <AppKit/NSWindow.h>
struct RHINOAppleSurfaceDesc {
    NSWindow* window;
};
#endif
