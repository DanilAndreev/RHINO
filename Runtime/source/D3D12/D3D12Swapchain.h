#pragma once

#ifdef ENABLE_API_D3D12

#include <dxgi1_4.h>
#include "D3D12BackendTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Swapchain : public Swapchain {
    public:
        void Release() noexcept final;
        Texture2D* GetTexture() noexcept final;

    public:
        void Initialize(IDXGIFactory2* factory, ID3D12CommandQueue* defaultQueue, const SwapchainDesc& desc) noexcept;
        void Present() noexcept;

    private:
        IDXGISwapChain3* m_Swapchain = nullptr;
        std::vector<D3D12Texture2D*> m_Textures = {};
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
