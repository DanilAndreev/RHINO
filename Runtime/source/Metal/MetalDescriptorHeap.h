#pragma once

#ifdef ENABLE_API_METAL

#include <RHINOTypes.h>
#import <Metal/Metal.h>

namespace RHINO::APIMetal {

    class MetalDescriptorHeap : public DescriptorHeap {
    public:
        id<MTLBuffer> m_ArgBuf = nil;
        std::vector<id<MTLResource>> resources{};

    public:
        void WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept final;

        void WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept final;

        void WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept final;

        void WriteSRV(const WriteTLASDescriptorDesc& desc) noexcept final;
    };

}// namespace RHINO::APIMetal

#endif // ENABLE_API_METAL

