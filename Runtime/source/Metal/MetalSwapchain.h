#pragma once

#ifdef ENABLE_API_METAL

#import <QuartzCore/CAMetalLayer.h>
#include "MetalBackendTypes.h"

namespace RHINO::APIMetal {
    class MetalSwapchain : public Swapchain {
    public:
        void Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue, const SwapchainDesc& desc) noexcept;
        void Release() noexcept final;

    public:
        void Present(MetalTexture2D* texture, size_t width, size_t height) noexcept;

    public:
        id<MTLCommandQueue> m_Queue = nil;
        CAMetalLayer* m_CAMetalLayer = nullptr;
    };
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
