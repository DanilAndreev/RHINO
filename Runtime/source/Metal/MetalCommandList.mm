#include "MetalCommandList.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

#include "MetalConverters.h"

namespace RHINO::APIMetal {
    void MetalCommandList::Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue) noexcept {
        m_Device = device;
        m_Cmd = [queue commandBuffer];
    }

    void MetalCommandList::SubmitToQueue() noexcept {
        [m_Cmd commit];
    }

    void MetalCommandList::Release() noexcept {}


    void MetalCommandList::Dispatch(const DispatchDesc& desc) noexcept {
        id<MTLComputeCommandEncoder> encoder = [m_Cmd computeCommandEncoder];

        std::vector<id<MTLResource>> usedUAVs;
        std::vector<id<MTLResource>> usedCBVSRVs;
        std::vector<id<MTLResource>> usedSMPs;
        for (const DescriptorSpaceDesc& space: m_CurComputePSO->spaceDescs) {
            for (size_t i = 0; i < space.rangeDescCount; ++i) {
                size_t pos = space.rangeDescs[i].baseRegisterSlot + space.offsetInDescriptorsFromTableStart;
                switch (space.rangeDescs[i].rangeType) {
                    case DescriptorRangeType::CBV:
                    case DescriptorRangeType::SRV:
                        usedCBVSRVs.push_back(m_CBVSRVUAVHeap->resources[m_CBVSRVUAVHeapOffset + pos]);
                        break;
                    case DescriptorRangeType::UAV:
                        usedUAVs.push_back(m_CBVSRVUAVHeap->resources[m_CBVSRVUAVHeapOffset + pos]);
                        break;
                    case DescriptorRangeType::Sampler:
                        if (m_SamplerHeap)
                            usedSMPs.push_back(m_SamplerHeap->resources[m_SamplerHeapOffset + pos]);
                        break;
                }
            }
        }

        [encoder useResources:usedUAVs.data() count:usedUAVs.size() usage:MTLResourceUsageRead | MTLResourceUsageWrite];
        [encoder useResources:usedCBVSRVs.data()
                        count:usedCBVSRVs.size()
                        usage:MTLResourceUsageRead | MTLResourceUsageSample];

        auto size = MTLSizeMake(desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
        // TODO: validate correctness
        auto threadgroupSize = MTLSizeMake(m_CurComputePSO->pso.maxTotalThreadsPerThreadgroup, 1, 1);
        [encoder dispatchThreadgroups:size threadsPerThreadgroup:threadgroupSize];
        [encoder endEncoding];
    }

    void MetalCommandList::Draw() noexcept {}

    void MetalCommandList::SetComputePSO(ComputePSO* pso) noexcept {
        auto* metalPSO = static_cast<MetalComputePSO*>(pso);
        m_CurComputePSO = metalPSO;
    }

    void MetalCommandList::SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept {
        auto* metalCBVSRVUAVHeap = static_cast<MetalDescriptorHeap*>(CBVSRVUAVHeap);
        auto* metalSamplerHeap = static_cast<MetalDescriptorHeap*>(CBVSRVUAVHeap);

        id<MTLComputeCommandEncoder> encoder = [m_Cmd computeCommandEncoder];
        if (samplerHeap) {
            // [encoder setBuffer: ]
        }
        else {
            [encoder setBuffer:metalCBVSRVUAVHeap->m_ArgBuf offset:0 atIndex:kIRArgumentBufferBindPoint];
        }
    }

    void MetalCommandList::CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset,
                                      size_t size) noexcept {
        auto srcBuffer = static_cast<MetalBuffer*>(src);
        auto dstBuffer = static_cast<MetalBuffer*>(dst);
        id<MTLBlitCommandEncoder> encoder = [m_Cmd blitCommandEncoder];

        [encoder copyFromBuffer:srcBuffer->buffer
                   sourceOffset:srcOffset
                       toBuffer: dstBuffer->buffer
                destinationOffset:dstOffset
                             size:size];
        [encoder endEncoding];

    }
    BLAS* MetalCommandList::BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                      const char* name) noexcept {
        auto* result = new MetalBLAS{};
        auto* metalScratch = static_cast<MetalBuffer*>(scratchBuffer);

        auto* metalVertex = static_cast<MetalBuffer*>(desc.vertexBuffer);
        auto* metalIndex = static_cast<MetalBuffer*>(desc.indexBuffer);
        auto* metalTransform = static_cast<MetalBuffer*>(desc.transformBuffer);

        auto triangleGeoDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
        triangleGeoDesc.vertexBuffer = metalVertex->buffer;
        triangleGeoDesc.vertexBufferOffset = desc.vertexBufferStartOffset;
        triangleGeoDesc.vertexFormat = Convert::ToMTLMTLAttributeFormat(desc.vertexFormat);
        triangleGeoDesc.vertexStride = desc.vertexStride;
        triangleGeoDesc.indexBuffer = metalIndex->buffer;
        triangleGeoDesc.indexBufferOffset = desc.indexBufferStartOffset;
        triangleGeoDesc.indexType = MTLIndexTypeUInt32;
        triangleGeoDesc.triangleCount = desc.indexCount / 3;
        triangleGeoDesc.primitiveDataBuffer = nil;
        triangleGeoDesc.primitiveDataStride = 0;
        triangleGeoDesc.primitiveDataElementSize = 0;
        triangleGeoDesc.transformationMatrixBuffer = metalTransform->buffer;
        triangleGeoDesc.transformationMatrixBufferOffset = desc.transformBufferStartOffset;
        triangleGeoDesc.intersectionFunctionTableOffset = 0; // TODO <- take from desc
        triangleGeoDesc.label = [NSString stringWithUTF8String:name];

        auto geometryDescriptors = [NSMutableArray array];
        [geometryDescriptors addObject:triangleGeoDesc];

        auto accelerationStructureDescriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
        accelerationStructureDescriptor.geometryDescriptors = geometryDescriptors;

        MTLAccelerationStructureSizes sizes = [m_Device accelerationStructureSizesWithDescriptor:accelerationStructureDescriptor];

        result->accelerationStructure = [m_Device newAccelerationStructureWithSize:sizes.accelerationStructureSize];

        id<MTLAccelerationStructureCommandEncoder> encoder = [m_Cmd accelerationStructureCommandEncoder];
        [encoder buildAccelerationStructure:result->accelerationStructure
                                 descriptor:accelerationStructureDescriptor
                              scratchBuffer:metalScratch->buffer
                        scratchBufferOffset:scratchBufferStartOffset];
        [encoder endEncoding];
        return result;
    }

    TLAS* MetalCommandList::BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                      const char* name) noexcept {
        auto* result = new MetalTLAS{};
        auto* metalScratch = static_cast<MetalBuffer*>(scratchBuffer);

        id<MTLBuffer> instanceDescBuf = [m_Device newBufferWithLength:0
                                                              options:MTLResourceOptionCPUCacheModeDefault];

        auto asDescs = [NSMutableArray array];
        for (size_t i = 0; i < desc.blasInstancesCount; ++i) {
            const BLASInstanceDesc& instance = desc.blasInstances[i];
            auto* metalBLAS = static_cast<MetalBLAS*>(instance.blas);
            [asDescs addObject:metalBLAS->accelerationStructure];
        }

        auto accelerationStructureDescriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];
        accelerationStructureDescriptor.instanceCount = desc.blasInstancesCount;
        accelerationStructureDescriptor.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeDefault;
        accelerationStructureDescriptor.instancedAccelerationStructures = asDescs;

        accelerationStructureDescriptor.instanceDescriptorBuffer = instanceDescBuf;
        accelerationStructureDescriptor.instanceDescriptorBufferOffset = 0;
        accelerationStructureDescriptor.instanceDescriptorStride = 0;

        //TODO: apply transform from desc

        MTLAccelerationStructureSizes sizes = [m_Device accelerationStructureSizesWithDescriptor:accelerationStructureDescriptor];
        result->accelerationStructure = [m_Device newAccelerationStructureWithSize:sizes.accelerationStructureSize];

        id<MTLAccelerationStructureCommandEncoder> encoder = [m_Cmd accelerationStructureCommandEncoder];
        [encoder buildAccelerationStructure:result->accelerationStructure
                                 descriptor:accelerationStructureDescriptor
                              scratchBuffer:metalScratch->buffer
                        scratchBufferOffset:scratchBufferStartOffset];
        [encoder endEncoding];
        return result;
    }

    void MetalCommandList::DispatchRays(const DispatchRaysDesc& desc) noexcept {
        //TODO: implement
    }

    void MetalCommandList::BuildRTPSO(RTPSO* pso) noexcept {
        //TODO: implement
    }
} // namespace RHINO::APIMetal
