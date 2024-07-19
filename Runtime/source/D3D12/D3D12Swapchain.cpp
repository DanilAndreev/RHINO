#ifdef ENABLE_API_D3D12

#include "D3D12Swapchain.h"
#include "D3D12Converters.h"

namespace RHINO::APID3D12 {
    void D3D12Swapchain::Initialize(IDXGIFactory* factory, ID3D12Device* device, const SwapchainDesc& desc) noexcept {
        DXGI_SWAP_CHAIN_DESC swapchainDesc{};
        swapchainDesc.Flags = 0;
        swapchainDesc.Windowed = desc.windowed;
        swapchainDesc.BufferCount = 3;
        swapchainDesc.BufferDesc.Format = Convert::ToDXGIFormat(desc.format);
        swapchainDesc.BufferDesc.Width = 0;
        swapchainDesc.BufferDesc.Height = 0;
        swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapchainDesc.BufferDesc.RefreshRate = {};
        swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapchainDesc.OutputWindow = hWnd;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.SwapEffect = desc.swapEffect;

        factory->CreateSwapChain(device, &swapchainDesc, &m_Swapchain);
    }

    void D3D12Swapchain::Present() noexcept {
        m_Swapchain->Present();
    }

    void D3D12Swapchain::Release() noexcept {
        m_Swapchain->Release();
        delete this;
    }

    void D3D12Swapchain::GetTexture() noexcept {

    }
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
