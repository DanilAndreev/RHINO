#ifdef ENABLE_API_METAL

#include "MetalBackend.h"
#import "MetalBackendTypes.h"
#include "MetalCommandList.h"
#include "MetalDescriptorHeap.h"

#import <SCARTools/SCARComputePSOArchiveView.h>

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

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

        NSError* error = nil;
        auto emptyHandler = ^{};

        dispatch_data_t dispatchData = dispatch_data_create(desc.CS.bytecode, desc.CS.bytecodeSize,
                                                            dispatch_get_main_queue(), emptyHandler);
        id<MTLLibrary> lib = [m_Device newLibraryWithData:dispatchData error:&error];
        if (!lib || error) {
            //TODO: show error.
            return nullptr;
        }

        NSString* functionName = [NSString stringWithUTF8String:desc.debugName];
        id<MTLFunction> shaderModule = [lib newFunctionWithName:functionName];

        MTLComputePipelineDescriptor* descriptor = [[MTLComputePipelineDescriptor alloc] init];
        descriptor.computeFunction = shaderModule;

        auto handler = ^(id<MTLComputePipelineState> pso,
                         MTLComputePipelineReflection* r, NSError* e) {
          if (error) {
              // error = e;
              assert(0);
              //TODO: assign to outer scope.
          } else {
              result->pso = pso;
          }
        };

        [m_Device newComputePipelineStateWithDescriptor:descriptor
                                                options:0
                                      completionHandler:handler];
        return result;
    }

    void MetalBackend::ReleaseComputePSO(ComputePSO* pso) noexcept {}

    Buffer* MetalBackend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage,
                                            size_t structuredStride, const char* name) noexcept {
        auto* result = new MetalBuffer{};
        result->buffer = [m_Device newBufferWithLength:size options:0];
        [result->buffer setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }

    void MetalBackend::ReleaseBuffer(Buffer* buffer) noexcept { delete buffer; }

    Texture2D* MetalBackend::CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                             ResourceUsage usage, const char* name) noexcept {
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

//        result->m_ArgBuf = [m_Device newBufferWithLength:result->encoder.encodedLength * descriptorsCount options:0];
        result->m_ArgBuf = [m_Device newBufferWithLength:sizeof(IRDescriptorTableEntry) * descriptorsCount
                                                 options:MTLResourceStorageModeShared];
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

    ComputePSO* MetalBackend::CompileSCARComputePSO(const void* scar, uint32_t sizeInBytes,
                                                    const char* debugName) noexcept {
        //TODO: check lang
        const SCARTools::SCARComputePSOArchiveView view{scar, sizeInBytes, debugName};
        if (!view.IsValid()) {
            return nullptr;
        }
        return CompileComputePSO(view.GetDesc());
    }

    void* MetalBackend::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        auto* metalBuffer = static_cast<MetalBuffer*>(buffer);
        return metalBuffer->buffer.contents;
    }

    void MetalBackend::UnmapMemory(Buffer* buffer) noexcept {
        // NOOP
    }
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
