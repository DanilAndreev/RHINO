#ifdef ENABLE_API_D3D12

#include "D3D12Utils.h"
#include "D3D12Swapchain.h"
#include "D3D12Converters.h"
#include <RHINOSwapchainPlatform.h>

namespace RHINO::APID3D12 {
    void D3D12Swapchain::Initialize(IDXGIFactory2* factory, ID3D12Device* device, ID3D12CommandQueue* queue,
                                    const SwapchainDesc& desc) noexcept {
        m_Device = device;
        m_Queue = queue;
        auto* surfaceDesc = static_cast<RHINOWin32SurfaceDesc*>(desc.surfaceDesc);

        DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
        // TODO: add param for tearing.
        swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        swapchainDesc.Format = Convert::ToDXGIFormat(desc.format);
        swapchainDesc.Width = desc.width;
        swapchainDesc.Height = desc.height;
        swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapchainDesc.Stereo = false;
        swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapchainDesc.BufferCount = desc.buffersCount;
        swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchainDesc.SampleDesc.Count = 1;
        swapchainDesc.SampleDesc.Quality = 0;
        swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScrDesc{};
        fullScrDesc.Windowed = desc.windowed;
        fullScrDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
        fullScrDesc.RefreshRate = {};
        fullScrDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

        IDXGISwapChain1* resSwapchain1 = nullptr;
        RHINO_D3DS(factory->CreateSwapChainForHwnd(m_Queue, surfaceDesc->hWnd, &swapchainDesc, &fullScrDesc, nullptr, &resSwapchain1));
        resSwapchain1->QueryInterface(&m_Swapchain);

        m_Textures.resize(desc.buffersCount);
        m_RingAlloc.resize(desc.buffersCount);
        for (size_t i = 0; i < desc.buffersCount; ++i) {
            m_Textures[i] = new D3D12Texture2D{};
            m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_Textures[i]->texture));
            m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_RingAlloc[i]));
        }
    }

    void D3D12Swapchain::Present(D3D12Texture2D* textureToPresent, size_t width, size_t height) noexcept {
        auto* d3d12ImageToPresent = INTERPRET_AS<D3D12Texture2D*>(textureToPresent);
        uint32_t index = m_Swapchain->GetCurrentBackBufferIndex();
        D3D12Texture2D* backbuffer = m_Textures[index];
        ID3D12GraphicsCommandList* cmd;
        RHINO_D3DS(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_RingAlloc[index], nullptr, IID_PPV_ARGS(&cmd)));

        D3D12_RESOURCE_BARRIER barrierToCopy{};
        barrierToCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierToCopy.Transition.pResource = backbuffer->texture;
        barrierToCopy.Transition.Subresource = 0;
        barrierToCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrierToCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        cmd->ResourceBarrier(1, &barrierToCopy);

        D3D12_TEXTURE_COPY_LOCATION srcLoc{};
        srcLoc.pResource = d3d12ImageToPresent->texture;
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION dstLoc{};
        dstLoc.pResource = backbuffer->texture;
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;
        D3D12_BOX box{};
        box.front = 0;
        box.back = 1;
        box.left = 0;
        box.right = width;
        box.top = 0;
        box.bottom = height;
        cmd->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &box);

        D3D12_RESOURCE_BARRIER barrierToPresent{};
        barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierToPresent.Transition.pResource = backbuffer->texture;
        barrierToPresent.Transition.Subresource = 0;
        barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        cmd->ResourceBarrier(1, &barrierToPresent);
        RHINO_D3DS(cmd->Close());
        ID3D12CommandList* generalCMDInterface;
        RHINO_D3DS(cmd->QueryInterface(&generalCMDInterface));
        m_Queue->ExecuteCommandLists(1, &generalCMDInterface);
        cmd->Release();
        RHINO_D3DS(m_Swapchain->Present(1, 0));
    }

    void D3D12Swapchain::Release() noexcept {
        m_Swapchain->Release();
        delete this;
    }
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
