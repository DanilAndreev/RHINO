#ifdef ENABLE_API_METAL

#import <Metal/Metal.h>

#import "MetalBackendTypes.h"
#include "MetalBackend.h"
#include "MetalDescriptorHeap.h"
#include "MetalCommandList.h"


namespace RHINO::APIMetal {
    void MetalBackend::Initialize() noexcept {
        m_DefaultQueue = [m_Device newCommandQueue];
        m_AsyncComputeQueue = [m_Device newCommandQueue];
        m_CopyQueue = [m_Device newCommandQueue];
    }
    void MetalBackend::Release() noexcept {
    }

    RTPSO* APIMetal::MetalBackend::CompileRTPSO(const RTPSODesc& desc) noexcept {
        return nullptr;
    }
    void MetalBackend::ReleaseRTPSO(RTPSO* pso) noexcept {
    }
    ComputePSO* MetalBackend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        return nullptr;
    }
    void MetalBackend::ReleaseComputePSO(ComputePSO* pso) noexcept {
    }
    MetalBuffer* MetalBackend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept {
        auto* result = new MetalBuffer{};
        result->buffer = [m_Device newBufferWithLength:size options: 0];
        [result->buffer setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }
    void MetalBackend::ReleaseBuffer(Buffer* buffer) noexcept {
        delete buffer;
    }
    MetalTexture2D* MetalBackend::CreateTexture2D() noexcept {
        auto* result = new MetalTexture2D{};
        MTLTextureDescriptor* descriptor = [[MTLTextureDescriptor alloc] init];

        result->texture = [m_Device newTextureWithDescriptor:descriptor];
        return result;
    }
    void MetalBackend::ReleaseTexture2D(Texture2D* texture) noexcept {
        delete texture;
    }
    DescriptorHeap* MetalBackend::CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept {
        auto* result = new MetalDescriptorHeap{};

        result->resources.resize(descriptorsCount);

        MTLArgumentDescriptor* arg = [MTLArgumentDescriptor argumentDescriptor];
        arg.index = 0;
        arg.access = MTLArgumentAccessReadOnly;
        switch (type) {
            case DescriptorHeapType::Sampler:
                arg.dataType = MTLDataTypeSampler;
            case DescriptorHeapType::RTV:
            case DescriptorHeapType::DSV:
            case DescriptorHeapType::SRV_CBV_UAV:
                arg.dataType = MTLDataTypePointer;
        }

        result->encoder = [m_Device newArgumentEncoderWithArguments:@[arg]];
        result->argBuf = [m_Device newBufferWithLength:result->encoder.encodedLength*descriptorsCount
                                                       options:0];
        [result->argBuf setLabel:[NSString stringWithUTF8String:name]];

        // TO WRITE
/*
        [result->encoder setArgumentBuffer:result->argBuf offset:i * result->encoder.encodedLength];
        [result->encoder setBuffer:target offset:0 atIndex:0];
*/

        return result;
    }
    void MetalBackend::ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept {
    }

    CommandList* MetalBackend::AllocateCommandList(const char* name) noexcept {
        auto* result = new MetalCommandList{};
        result->cmd = [m_DefaultQueue commandBuffer];
        return result;
    }

    void MetalBackend::ReleaseCommandList(CommandList* commandList) noexcept {
        delete commandList;
    }

    void MetalBackend::SubmitCommandList(CommandList* cmd) noexcept {
    }

}// namespace RHINO::APIMetal

#endif// ENABLE_API_METAL
