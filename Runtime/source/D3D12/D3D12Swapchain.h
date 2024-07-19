#pragma once

#ifdef ENABLE_API_D3D12

#include <dxgi.h>
#include "D3D12BackendTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Swapchain : public Swapchain {
    public:
        void Release() noexcept final;
        void GetTexture() noexcept final;

    public:
        void Initialize(IDXGIFactory* factory, ID3D12Device* device, const SwapchainDesc& desc) noexcept;
        void Present() noexcept;

    private:
        IDXGISwapChain* m_Swapchain = nullptr;
        uint32_t m_CurrentImageIndex = 0;
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
