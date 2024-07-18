#pragma once

#ifdef ENABLE_API_METAL

#include <RHINOTypes.h>
#import <Metal/Metal.h>

namespace RHINO::APIMetal {

    class MetalDescriptorHeap : public DescriptorHeap {
    public:
        id<MTLBuffer> m_DescriptorHeap = nil;
        std::vector<id<MTLResource>> m_Resources{};

    public:
        void WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept final;
        void WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept final;

        void WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept final;

        void WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept final;
        void WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept final;

        void WriteSRV(const WriteTLASDescriptorDesc& desc) noexcept final;

        void WriteSMP(RHINO::Sampler *sampler, size_t offsetInHeap) noexcept final;

    public:
        id<MTLBuffer> GetHeapBuffer() noexcept;
        size_t GetDescriptorStride() const noexcept;
        const std::vector<id<MTLResource>>& GetBoundResources() const noexcept;

    public:
        void Release() noexcept final;
    };

}// namespace RHINO::APIMetal

#endif // ENABLE_API_METAL

