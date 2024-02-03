#ifdef ENABLE_API_METAL

#include "MetalBackend.h"
#import "MetalBackendTypes.h"
#include "MetalCommandList.h"
#include "MetalDescriptorHeap.h"

#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

namespace RHINO::APIMetal {
    void MetalBackend::Initialize() noexcept {
        m_DefaultQueue = [m_Device newCommandQueue];
        m_AsyncComputeQueue = [m_Device newCommandQueue];
        m_CopyQueue = [m_Device newCommandQueue];
    }
    void MetalBackend::Release() noexcept {}

    RTPSO* APIMetal::MetalBackend::CompileRTPSO(const RTPSODesc& desc) noexcept { return nullptr; }
    void MetalBackend::ReleaseRTPSO(RTPSO* pso) noexcept {}
    ComputePSO* MetalBackend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* result = new MetalComputePSO{};

        result->spaceDescs.resize(desc.spacesCount);
        for (size_t space = 0; space < desc.spacesCount; ++space) {
            result->spaceDescs[space] = desc.spacesDescs[space];
            auto* rangeDescsStart = &(*result->rangeDescsStorage.rbegin()) + 1;
            for (size_t i = 0; i < desc.spacesDescs[space].rangeDescCount; ++i) {
                result->rangeDescsStorage.push_back(desc.spacesDescs[space].rangeDescs[i]);
            }
            result->spaceDescs[space].rangeDescs = rangeDescsStart;
        }

        MTLComputePipelineDescriptor* descriptor = [[MTLComputePipelineDescriptor alloc] init];
        NSError* error = nil;
        [m_Device newComputePipelineStateWithDescriptor:descriptor
                                                options:0
                                      completionHandler:^(id<MTLComputePipelineState> pso,
                                                          MTLComputePipelineReflection* r, NSError* e) {
                                        if (error) {
                                            // error = e;
                                            assert(0);
                                            //TODO: assing to uoter scope.
                                        } else {
                                            result->pso = pso;
                                        }
                                      }];
        return result;
    }
    void MetalBackend::ReleaseComputePSO(ComputePSO* pso) noexcept {}
    MetalBuffer* MetalBackend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage,
                                            size_t structuredStride, const char* name) noexcept {
        auto* result = new MetalBuffer{};
        result->buffer = [m_Device newBufferWithLength:size options:0];
        [result->buffer setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }
    void MetalBackend::ReleaseBuffer(Buffer* buffer) noexcept { delete buffer; }
    MetalTexture2D* MetalBackend::CreateTexture2D() noexcept {
        auto* result = new MetalTexture2D{};
        MTLTextureDescriptor* descriptor = [[MTLTextureDescriptor alloc] init];

        result->texture = [m_Device newTextureWithDescriptor:descriptor];
        return result;
    }
    void MetalBackend::ReleaseTexture2D(Texture2D* texture) noexcept { delete texture; }
    DescriptorHeap* MetalBackend::CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount,
                                                       const char* name) noexcept {
        auto* result = new MetalDescriptorHeap{};

        result->resources.resize(descriptorsCount);

        result->m_ArgBuf = [m_Device newBufferWithLength:result->encoder.encodedLength * descriptorsCount options:0];
        [result->m_ArgBuf setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }
    void MetalBackend::ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept {}

    CommandList* MetalBackend::AllocateCommandList(const char* name) noexcept {
        auto* result = new MetalCommandList{};
        result->cmd = [m_DefaultQueue commandBuffer];
        return result;
    }

    void MetalBackend::ReleaseCommandList(CommandList* commandList) noexcept { delete commandList; }

    void MetalBackend::SubmitCommandList(CommandList* cmd) noexcept {}

} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
