#pragma once

#ifdef ENABLE_API_METAL

#include <RHINOTypes.h>
#import <Metal/Metal.h>

namespace RHINO::APIMetal {

    class MetalDescriptorHeap : public DescriptorHeap {
    public:
        id<MTLBuffer> argBuf = nil;
        id<MTLArgumentEncoder> encoder = nil;
        std::vector<id<MTLResource>> resources{};

    public:
        void WriteSRV(const WriteBufferSRVDesc& desc) noexcept final;
        void WriteUAV(const WriteBufferSRVDesc& desc) noexcept final;
        void WriteCBV(const WriteBufferSRVDesc& desc) noexcept final;

        void WriteSRV(const WriteTexture2DSRVDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture2DSRVDesc& desc) noexcept final;

        void WriteSRV(const WriteTexture3DSRVDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture3DSRVDesc& desc) noexcept final;

    };

}// namespace RHINO::APIMetal

#endif // ENABLE_API_METAL

