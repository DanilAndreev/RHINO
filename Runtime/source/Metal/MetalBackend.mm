#ifdef ENABLE_API_METAL

#include "MetalBackend.h"
#import "MetalBackendTypes.h"
#include "MetalCommandList.h"
#include "MetalDescriptorHeap.h"
#include "MetalConverters.h"

#import <SCARTools/SCARComputePSOArchiveView.h>

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

namespace RHINO::APIMetal {
    void MetalBackend::Initialize() noexcept {
        m_Device = MTLCopyAllDevices()[0];
        m_DefaultQueue = [m_Device newCommandQueue];
        m_AsyncComputeQueue = [m_Device newCommandQueue];
        m_CopyQueue = [m_Device newCommandQueue];
    }

    void MetalBackend::Release() noexcept {}

    RTPSO* APIMetal::MetalBackend::CreateRTPSO(const RHINO::RTPSODesc& desc) noexcept { return nullptr; }

    ComputePSO* MetalBackend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* result = new MetalComputePSO{};

        result->spaceDescs.resize(desc.spacesCount);
        std::vector<size_t> offsetInRangeDescsPerDescriptorSpace{};
        for (size_t spaceID = 0; spaceID < desc.spacesCount; ++spaceID) {
            result->spaceDescs[spaceID] = desc.spacesDescs[spaceID];
            offsetInRangeDescsPerDescriptorSpace.push_back(result->rangeDescsStorage.size());
            for (size_t i = 0; i < desc.spacesDescs[spaceID].rangeDescCount; ++i) {
                result->rangeDescsStorage.push_back(desc.spacesDescs[spaceID].rangeDescs[i]);
            }
        }
        for (size_t spaceID = 0; spaceID < desc.spacesCount; ++spaceID) {
            auto addr = result->rangeDescsStorage.data() + offsetInRangeDescsPerDescriptorSpace[spaceID];
            result->spaceDescs[spaceID].rangeDescs = addr;
        }

        NSError* error = nil;
        auto emptyHandler = ^{};

        dispatch_data_t dispatchData = dispatch_data_create(desc.CS.bytecode, desc.CS.bytecodeSize,
                                                            dispatch_get_main_queue(), emptyHandler);
        id<MTLLibrary> lib = [m_Device newLibraryWithData:dispatchData error:&error];
        if (!lib || error) {
            assert(0);
            //TODO: show error.
            return nullptr;
        }

        NSString* functionName = [NSString stringWithUTF8String:desc.CS.entrypoint];
        id<MTLFunction> shaderModule = [lib newFunctionWithName:functionName];

        MTLComputePipelineDescriptor* descriptor = [[MTLComputePipelineDescriptor alloc] init];
        descriptor.computeFunction = shaderModule;
        result->pso = [m_Device newComputePipelineStateWithDescriptor:descriptor options:0 reflection:nil error:&error];
        return result;
    }

    Buffer* MetalBackend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage,
                                            size_t structuredStride, const char* name) noexcept {
        auto* result = new MetalBuffer{};
        result->buffer = [m_Device newBufferWithLength:size options:0];
        [result->buffer setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }

    Texture2D* MetalBackend::CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                             ResourceUsage usage, const char* name) noexcept {
        auto* result = new MetalTexture2D{};
        MTLTextureDescriptor* descriptor = [[MTLTextureDescriptor alloc] init];

        result->texture = [m_Device newTextureWithDescriptor:descriptor];
        return result;
    }

    DescriptorHeap* MetalBackend::CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount,
                                                       const char* name) noexcept {
        auto* result = new MetalDescriptorHeap{};

        result->m_Resources.resize(descriptorsCount);

//        result->m_DescriptorHeap = [m_Device newBufferWithLength:result->encoder.encodedLength * descriptorsCount options:0];
        result->m_DescriptorHeap = [m_Device newBufferWithLength:sizeof(IRDescriptorTableEntry) * descriptorsCount
                                                 options:MTLResourceStorageModeShared];
//        result->m_DescriptorHeap = [m_Device newBufferWithLength:sizeof(uint64_t) * descriptorsCount
//                                                 options:MTLResourceStorageModeShared];
        [result->m_DescriptorHeap setLabel:[NSString stringWithUTF8String:name]];
        return result;
    }

    CommandList* MetalBackend::AllocateCommandList(const char* name) noexcept {
        auto* result = new MetalCommandList{};
        result->Initialize(m_Device, m_DefaultQueue);
        return result;
    }

    void MetalBackend::SubmitCommandList(CommandList* cmd) noexcept {
        auto* metalCmd = static_cast<MetalCommandList*>(cmd);
        metalCmd->SubmitToQueue();
    }

    void* MetalBackend::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        auto* metalBuffer = static_cast<MetalBuffer*>(buffer);
        return metalBuffer->buffer.contents;
    }

    void MetalBackend::UnmapMemory(Buffer* buffer) noexcept {
        // NOOP
    }

    ASPrebuildInfo MetalBackend::GetBLASPrebuildInfo(const BLASDesc& desc) noexcept {
        auto triangleGeoDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
        triangleGeoDesc.vertexBuffer = nil;
        triangleGeoDesc.vertexBufferOffset = 0;
        triangleGeoDesc.vertexFormat = Convert::ToMTLMTLAttributeFormat(desc.vertexFormat);
        triangleGeoDesc.vertexStride = desc.vertexStride;
        triangleGeoDesc.indexBuffer = nil;
        triangleGeoDesc.indexBufferOffset = 0;
        triangleGeoDesc.indexType = Convert::ToMTLIndexType(desc.indexFormat);
        triangleGeoDesc.triangleCount = desc.indexCount / 3;
        triangleGeoDesc.primitiveDataBuffer = nil;
        triangleGeoDesc.primitiveDataStride = 0;
        triangleGeoDesc.primitiveDataElementSize = 0;
        triangleGeoDesc.transformationMatrixBuffer = nil;
        triangleGeoDesc.transformationMatrixBufferOffset = 0;

        auto accelerationStructureDescriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
        auto geometryDescriptors = [NSMutableArray array];
        [geometryDescriptors addObject:triangleGeoDesc];

        accelerationStructureDescriptor.geometryDescriptors = geometryDescriptors;
        MTLAccelerationStructureSizes sizes = [m_Device accelerationStructureSizesWithDescriptor:accelerationStructureDescriptor];

        ASPrebuildInfo result{};
        result.MaxASSizeInBytes = sizes.accelerationStructureSize;
        result.MaxASSizeInBytes = sizes.buildScratchBufferSize;
        return result;
    }

    ASPrebuildInfo MetalBackend::GetTLASPrebuildInfo(const TLASDesc& desc) noexcept {

    }

    Semaphore* MetalBackend::CreateSyncSemaphore(uint64_t initialValue) noexcept {
        auto* result = new MetalSemaphore{};
        result->event = [m_Device newSharedEvent];
        return result;
    }

    void MetalBackend::SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept {
        auto* metalSemaphore = static_cast<MetalSemaphore*>(semaphore);
        id<MTLCommandBuffer> cmd = [m_DefaultQueue commandBuffer];
        [cmd encodeSignalEvent: metalSemaphore->event value: value];
        [cmd commit];
    }

    void MetalBackend::SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept {
        auto* metalSemaphore = static_cast<MetalSemaphore*>(semaphore);
        [metalSemaphore->event setSignaledValue: value];
    }

    bool MetalBackend::SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept {
        const auto* metalSemaphore = static_cast<const MetalSemaphore*>(semaphore);
        // TODO: Uncommit after MacOS 15 [:harold:]
        // return [metalSemaphore->event waitUntilSignaledValue:value timeoutMS:timeout];
        return true;
    }

    void MetalBackend::SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept {
        const auto* metalSemaphore = static_cast<const MetalSemaphore*>(semaphore);
        id<MTLCommandBuffer> cmd = [m_DefaultQueue commandBuffer];
        [cmd encodeWaitForEvent: metalSemaphore->event value: value];
        [cmd commit];
    }

    uint64_t MetalBackend::GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept {
        const auto* metalSemaphore = static_cast<const MetalSemaphore*>(semaphore);
        [metalSemaphore->event signaledValue];
        return true;
    }
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
