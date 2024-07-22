#pragma once

#ifdef ENABLE_API_D3D12

#include <dxgi1_4.h>
#include "D3D12BackendTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Swapchain : public Swapchain {
    public:
        void Release() noexcept final;

    public:
        void Initialize(IDXGIFactory2* factory, ID3D12Device* device, ID3D12CommandQueue* queue, const SwapchainDesc& desc) noexcept;
        void Present(D3D12Texture2D* textureToPresent, size_t width, size_t height) noexcept;

    private:
        ID3D12Device* m_Device = nullptr;
        ID3D12CommandQueue* m_Queue = nullptr;
        IDXGISwapChain3* m_Swapchain = nullptr;
        std::vector<D3D12Texture2D*> m_Textures = {};
        std::vector<ID3D12CommandAllocator*> m_RingAlloc = {};
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
