#ifdef ENABLE_API_METAL

#include <RHINOSwapchainPlatform.h>
#include "MetalSwapchain.h"
#include "MetalConverters.h"

namespace RHINO::APIMetal {
    void MetalSwapchain::Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue, const SwapchainDesc& desc) noexcept {
        auto* surfaceDesc = static_cast<RHINOAppleSurfaceDesc*>(desc.surfaceDesc);
        m_Queue = queue;
        m_CAMetalLayer = [CAMetalLayer layer];
        m_CAMetalLayer.device = device;
        m_CAMetalLayer.pixelFormat = Convert::ToMTLPixelFormat(desc.format);
        m_CAMetalLayer.displaySyncEnabled = true;
        m_CAMetalLayer.framebufferOnly = false;
        m_CAMetalLayer.drawableSize = CGSizeMake(desc.width, desc.height);

        surfaceDesc->window.contentView.layer = m_CAMetalLayer;
        surfaceDesc->window.contentView.wantsLayer = true;
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
