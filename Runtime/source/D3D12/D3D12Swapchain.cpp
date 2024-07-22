#ifdef ENABLE_API_D3D12

#include "D3D12Utils.h"
#include "D3D12Swapchain.h"
#include "D3D12Converters.h"

namespace RHINO::APID3D12 {
    void D3D12Swapchain::Initialize(IDXGIFactory2* factory, ID3D12CommandQueue* defaultQueue, const SwapchainDesc& desc) noexcept {
        auto* surfaceDesc = static_cast<Win32SurfaceDesc*>(desc.surfaceDesc);

        // DXGI_SWAP_CHAIN_DESC swapchainDesc{};
        // swapchainDesc.Flags = 0;
        // swapchainDesc.Windowed = desc.windowed;
        // swapchainDesc.BufferCount = desc.buffersCount;
        // swapchainDesc.BufferDesc.Format = Convert::ToDXGIFormat(desc.format);
        // swapchainDesc.BufferDesc.Width = 0;
        // swapchainDesc.BufferDesc.Height = 0;
        // swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        // swapchainDesc.BufferDesc.RefreshRate = {};
        // swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        // swapchainDesc.OutputWindow = surfaceDesc->hWnd;
        // swapchainDesc.SampleDesc.Count = 1;
        // swapchainDesc.SampleDesc.Quality = 0;
        // swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        // RHINO_D3DS(factory->CreateSwapChain(device, &swapchainDesc, &m_Swapchain));



        DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
        // TODO: add param for tearing.
        swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        swapchainDesc.Format = Convert::ToDXGIFormat(desc.format);
        swapchainDesc.Width = desc.width;
        swapchainDesc.Height = desc.height;
        swapchainDesc.Scaling = DXGI_SCALING_NONE;
        swapchainDesc.Stereo = false;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchainDesc.BufferCount = desc.buffersCount;
        swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScrDesc{};
        fullScrDesc.Windowed = true;

        IDXGISwapChain1* resSwapchain1 = nullptr;
        RHINO_D3DS(factory->CreateSwapChainForHwnd(defaultQueue, surfaceDesc->hWnd, &swapchainDesc, &fullScrDesc, nullptr, &resSwapchain1));
        resSwapchain1->QueryInterface(&m_Swapchain);

        m_Textures.resize(desc.buffersCount);
        for (size_t i = 0; i < desc.buffersCount; ++i) {
            m_Textures[i] = new D3D12Texture2D{};
            m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_Textures[i]->texture));
        }
    }

    void D3D12Swapchain::Present() noexcept {
        m_Swapchain->Present(1, 0);
    }

    void D3D12Swapchain::Release() noexcept {
        m_Swapchain->Release();
        delete this;
    }

    Texture2D* D3D12Swapchain::GetTexture() noexcept {
        auto* backbuffer = m_Textures[m_Swapchain->GetCurrentBackBufferIndex()];
        return backbuffer;
    }
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
