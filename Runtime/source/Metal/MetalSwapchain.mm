#ifdef ENABLE_API_METAL

#include <RHINOSwapchainPlatform.h>
#include "MetalSwapchain.h"

namespace RHINO::APIMetal {
    void MetalSwapchain::Initialize(id<MTLCommandQueue> queue, const SwapchainDesc& desc) noexcept {
        auto* surfaceDesc = static_cast<AppleSurfaceDesc*>(desc.surfaceDesc);
        m_Queue = queue;
        m_CAMetalLayer = surfaceDesc->layer;
    }

    void MetalSwapchain::Release() noexcept { delete this; }

    void MetalSwapchain::Present(MetalTexture2D* texture, size_t width, size_t height) noexcept {
        id<CAMetalDrawable> drawable = [m_CAMetalLayer nextDrawable];
        id<MTLTexture> backbuffer = [drawable texture];
        id<MTLCommandBuffer> cmd = [m_Queue commandBuffer];
        id<MTLBlitCommandEncoder> encoder = [cmd blitCommandEncoder];
        [encoder copyFromTexture:texture->texture
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake(0, 0, 0)
                       sourceSize:MTLSizeMake(width, height, 1)
                        toTexture:backbuffer
                 destinationSlice:0
                 destinationLevel:0
                destinationOrigin:MTLOriginMake(0, 0, 0)];
        [encoder endEncoding];
        [cmd commit];
        [drawable present];
    }
} // namespace RHINO::APIMetal

#endif ENABLE_API_METAL
