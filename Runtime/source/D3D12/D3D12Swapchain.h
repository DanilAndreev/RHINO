#pragma once

#ifdef ENABLE_API_D3D12

#include <dxgi.h>
#include "D3D12BackendTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Swapchain : public Swapchain {
    public:


    public:
        IDXGISwapChain* m_Swapchain = nullptr;
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
