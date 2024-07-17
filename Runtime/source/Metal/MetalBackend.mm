#ifdef ENABLE_API_METAL

#include "MetalBackend.h"
#import "MetalBackendTypes.h"
#include "MetalCommandList.h"
#include "MetalDescriptorHeap.h"
#include "MetalConverters.h"
#include "MetalUtils.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

namespace RHINO::APIMetal {
    void MetalBackend::Initialize() noexcept {
        m_IRCompiler = IRCompilerCreate();

        m_Device = MTLCopyAllDevices()[0];
        m_DefaultQueue = [m_Device newCommandQueue];
        m_AsyncComputeQueue = [m_Device newCommandQueue];
        m_CopyQueue = [m_Device newCommandQueue];

        {
            NSError* error = nil;
            auto manager = [MTLCaptureManager sharedCaptureManager];
            MTLCaptureDescriptor* captureDescriptor = [[MTLCaptureDescriptor alloc] init];
            captureDescriptor.destination = MTLCaptureDestinationDeveloperTools;
            captureDescriptor.captureObject = m_Device;
            [manager startCaptureWithDescriptor:captureDescriptor error:&error];
        }
    }

    void MetalBackend::Release() noexcept {
        IRCompilerDestroy(m_IRCompiler);
        m_IRCompiler = nullptr;

        auto manager= [MTLCaptureManager sharedCaptureManager];
        [manager stopCapture];
    }

    RootSignature* MetalBackend::SerializeRootSignature(const RootSignatureDesc& desc) noexcept {
        IRError* pError = nullptr;
        auto* result = new MetalRootSignature{};

        // TODO: if root constants defined: add root constants root param

        std::vector<IRDescriptorRange1> rangeDescsStorage{};
        std::vector<IRRootParameter1> rootParamsDescs{};
        rootParamsDescs.reserve(desc.spacesCount + 1);
        std::vector<size_t> offsetsInRangeDescsPerSpaceIdx{};
        offsetsInRangeDescsPerSpaceIdx.resize(desc.spacesCount);

        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            IRRootParameter1& rootParamDesc = rootParamsDescs.emplace_back();
            rootParamDesc.ParameterType = IRRootParameterTypeDescriptorTable;
            rootParamDesc.DescriptorTable.NumDescriptorRanges = desc.spacesDescs[spaceIdx].rangeDescCount;
            offsetsInRangeDescsPerSpaceIdx[spaceIdx] = rangeDescsStorage.size();

            //TODO: assert that ranges in one space exclusively CBVSRVUAV or SMP.

            for (size_t i = 0; i < desc.spacesDescs[spaceIdx].rangeDescCount; ++i) {
                IRDescriptorRange1& rangeDesc = rangeDescsStorage.emplace_back();
                rangeDesc.NumDescriptors = desc.spacesDescs[spaceIdx].rangeDescs[i].descriptorsCount;
                rangeDesc.RegisterSpace = desc.spacesDescs[spaceIdx].space;
                rangeDesc.OffsetInDescriptorsFromTableStart = desc.spacesDescs[spaceIdx].offsetInDescriptorsFromTableStart +
                                                              desc.spacesDescs[spaceIdx].rangeDescs[i].baseRegisterSlot;
                rangeDesc.BaseShaderRegister = desc.spacesDescs[spaceIdx].rangeDescs[i].baseRegisterSlot;
                switch (desc.spacesDescs[spaceIdx].rangeDescs[i].rangeType) {
                    case RHINO::DescriptorRangeType::SRV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeSRV;
                        break;
                    case RHINO::DescriptorRangeType::UAV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeUAV;
                        break;
                    case RHINO::DescriptorRangeType::CBV:
                        rangeDesc.RangeType = IRDescriptorRangeTypeCBV;
                        break;
                    case RHINO::DescriptorRangeType::Sampler:
                        rangeDesc.RangeType = IRDescriptorRangeTypeSampler;
                        break;
                }
            }
        }
        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            auto* rangesPtr = rangeDescsStorage.data() + offsetsInRangeDescsPerSpaceIdx[spaceIdx];
            rootParamsDescs[spaceIdx].DescriptorTable.pDescriptorRanges = rangesPtr;
        }

        IRRootSignatureDescriptor1 rootSignatureDesc{};
        rootSignatureDesc.NumParameters = rootParamsDescs.size();
        rootSignatureDesc.pParameters = rootParamsDescs.data();
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = IRRootSignatureFlags(IRRootSignatureFlagDenyHullShaderRootAccess |
                                                       IRRootSignatureFlagDenyDomainShaderRootAccess |
                                                       IRRootSignatureFlagDenyGeometryShaderRootAccess);

        IRVersionedRootSignatureDescriptor rsVersionedDesc{};
        rsVersionedDesc.version = IRRootSignatureVersion_1_1;
        rsVersionedDesc.desc_1_1 = rootSignatureDesc;
        result->rootSignature = IRRootSignatureCreateFromDescriptor(&rsVersionedDesc, &pError);
        if (pError) {
            return nullptr;
        }

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

        return result;
    }

    RTPSO* APIMetal::MetalBackend::CreateRTPSO(const RHINO::RTPSODesc& desc) noexcept { return nullptr; }

    ComputePSO* MetalBackend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* metalRootSignature = INTERPRET_AS<MetalRootSignature*>(desc.rootSignature);

        IRError* pError = nullptr;
        auto* result = new MetalComputePSO{};

        IRCompilerSetGlobalRootSignature(m_IRCompiler, metalRootSignature->rootSignature);
        IRCompilerSetEntryPointName(m_IRCompiler, desc.CS.entrypoint);
        IRObject* pDXIL = IRObjectCreateFromDXIL(desc.CS.bytecode, desc.CS.bytecodeSize, IRBytecodeOwnershipNone);
        IRObject* outIR = IRCompilerAllocCompileAndLink(m_IRCompiler, desc.CS.entrypoint, pDXIL, &pError);

        if (!outIR) {
            // Inspect pError to determine cause.
            assert(0);
            IRErrorDestroy(pError);
            return nullptr;
        }

        // Retrieve Metallib:
        IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();
        IRObjectGetMetalLibBinary(outIR, IRShaderStageCompute, pMetallib);

        IRObjectDestroy(pDXIL);
        IRObjectDestroy(outIR);

        NSError* error = nil;
        auto emptyHandler = ^{};

        dispatch_data_t dispatchData = IRMetalLibGetBytecodeData(pMetallib);
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

        IRMetalLibBinaryDestroy(pMetallib);
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
        descriptor.arrayLength = 1;
        descriptor.mipmapLevelCount = mips;
        descriptor.width = dimensions.width;
        descriptor.height = dimensions.height;
        descriptor.depth = 1;
        descriptor.cpuCacheMode = MTLCPUCacheModeDefaultCache;
        descriptor.pixelFormat = Convert::ToMTLPixelFormat(format);
        descriptor.resourceOptions = 0;
        descriptor.sampleCount = 1;
        descriptor.textureType = MTLTextureType2DArray;
        descriptor.storageMode = MTLStorageModePrivate;
        descriptor.usage = Convert::ToMTLResourceUsage(usage);

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
        auto* metalCmd = INTERPRET_AS<MetalCommandList*>(cmd);
        metalCmd->SubmitToQueue();
    }

    void* MetalBackend::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        auto* metalBuffer = INTERPRET_AS<MetalBuffer*>(buffer);
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
        [result->event setSignaledValue: initialValue];
        return result;
    }

    void MetalBackend::SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept {
        auto* metalSemaphore = INTERPRET_AS<MetalSemaphore*>(semaphore);
        id<MTLCommandBuffer> cmd = [m_DefaultQueue commandBuffer];
        [cmd encodeSignalEvent: metalSemaphore->event value: value];
        [cmd commit];
    }

    void MetalBackend::SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept {
        auto* metalSemaphore = INTERPRET_AS<MetalSemaphore*>(semaphore);
        [metalSemaphore->event setSignaledValue: value];
    }

    bool MetalBackend::SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept {
        const auto* metalSemaphore = INTERPRET_AS<const MetalSemaphore*>(semaphore);
        return WaitForMTLSharedEventValue(metalSemaphore->event, value, timeout);
    }

    void MetalBackend::SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept {
        const auto* metalSemaphore = INTERPRET_AS<const MetalSemaphore*>(semaphore);
        id<MTLCommandBuffer> cmd = [m_DefaultQueue commandBuffer];
        [cmd encodeWaitForEvent: metalSemaphore->event value: value];
        [cmd commit];
    }

    uint64_t MetalBackend::GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept {
        const auto* metalSemaphore = INTERPRET_AS<const MetalSemaphore*>(semaphore);
        [metalSemaphore->event signaledValue];
        return true;
    }
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
