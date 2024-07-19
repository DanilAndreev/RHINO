#pragma once

#ifdef ENABLE_API_D3D12

#include <dxgi.h>
#include "D3D12BackendTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Swapchain : public Swapchain {
    public:
        void Initialize(IDXGIFactory* factory, ID3D12Device* device, const SwapchainDesc& desc) noexcept;
        void Release() noexcept final;

    private:
        IDXGISwapChain* m_Swapchain = nullptr;
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
