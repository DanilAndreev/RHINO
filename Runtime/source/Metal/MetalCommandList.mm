#ifdef ENABLE_API_METAL

#include "MetalCommandList.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

#include "MetalConverters.h"
#include "MetalUtils.h"


namespace RHINO::APIMetal {

    void MetalCommandList::Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue) noexcept {
        m_Device = device;
        m_RootSignaturesRing = [m_Device newBufferWithLength:sizeof(RootSignatureT) * ROOT_SIGNATURE_RING_SIZE
                                                     options:MTLResourceStorageModeManaged];
        [m_RootSignaturesRing setLabel: @"RootSignatureRing"];
        for (size_t i = 0; i < ROOT_SIGNATURE_RING_SIZE; ++i) {
            m_RootSignaturesRingSync[i] = [m_Device newSharedEvent];
            [m_RootSignaturesRingSync[i] setSignaledValue: 0];
        }

        m_Cmd = [queue commandBuffer];
    }

    void MetalCommandList::SubmitToQueue() noexcept {
        [m_Cmd commit];
    }

    void MetalCommandList::Release() noexcept {
        delete this;
    }

    void MetalCommandList::SetRootSignature(RHINO::RootSignature* rootSignature) noexcept {
        m_CurRootSignature = INTERPRET_AS<MetalRootSignature*>(rootSignature);
    }

    void MetalCommandList::Dispatch(const DispatchDesc& desc) noexcept {
        id<MTLComputeCommandEncoder> encoder = [m_Cmd computeCommandEncoder];

        std::vector<id<MTLResource>> usedUAVs;
        std::vector<id<MTLResource>> usedCBVSRVs;
        std::vector<id<MTLResource>> usedSMPs;
        for (const DescriptorSpaceDesc& space: m_CurRootSignature->spaceDescs) {
            for (size_t spaceIdx = 0; spaceIdx < space.rangeDescCount; ++spaceIdx) {
                size_t pos = space.rangeDescs[spaceIdx].baseRegisterSlot + space.offsetInDescriptorsFromTableStart;
                switch (space.rangeDescs[spaceIdx].rangeType) {
                    case DescriptorRangeType::CBV:
                    case DescriptorRangeType::SRV: {
                        for (size_t i = 0; i < space.rangeDescs[spaceIdx].descriptorsCount; ++i) {
                            usedCBVSRVs.push_back(m_CBVSRVUAVHeap->m_Resources[m_CBVSRVUAVHeapOffset + pos + i]);
                        }
                        break;
                    }
                    case DescriptorRangeType::UAV: {
                        for (size_t i = 0; i < space.rangeDescs[spaceIdx].descriptorsCount; ++i) {
                            usedUAVs.push_back(m_CBVSRVUAVHeap->m_Resources[m_CBVSRVUAVHeapOffset + pos + i]);
                        }
                        break;
                    }
                    case DescriptorRangeType::Sampler: {
                        if (m_SamplerHeap) {
                            for (size_t i = 0; i < space.rangeDescs[spaceIdx].descriptorsCount; ++i) {
                                usedSMPs.push_back(m_SamplerHeap->m_Resources[m_SamplerHeapOffset + pos + i]);
                            }
                        }
                        break;
                    }
                }
            }
        }

        const size_t rootSignatureOffset = m_CurrentRingRootSignatureIndex * sizeof(RootSignatureT);
        [encoder setBuffer:m_RootSignaturesRing offset:rootSignatureOffset atIndex:kIRArgumentBufferBindPoint];
        [encoder useResource:m_RootSignaturesRing usage:MTLResourceUsageRead];
        m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex] += 1;


        [encoder setBuffer:m_CBVSRVUAVHeap->GetHeapBuffer() offset:0 atIndex:kIRDescriptorHeapBindPoint];
        [encoder useResource:m_CBVSRVUAVHeap->GetHeapBuffer() usage:MTLResourceUsageRead];
        if (m_SamplerHeap) {
            [encoder setBuffer:m_SamplerHeap->GetHeapBuffer() offset:0 atIndex:kIRSamplerHeapBindPoint];
            [encoder useResource:m_SamplerHeap->GetHeapBuffer() usage:MTLResourceUsageRead];
        }

        [encoder useResources:usedUAVs.data() count:usedUAVs.size() usage:MTLResourceUsageRead | MTLResourceUsageWrite];
        [encoder useResources:usedCBVSRVs.data() count:usedCBVSRVs.size() usage:MTLResourceUsageRead | MTLResourceUsageSample];

        auto size = MTLSizeMake(desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);

        auto threadgroupSize = MTLSizeMake(m_CurComputePSO->localWorkgroupSize[0], m_CurComputePSO->localWorkgroupSize[1],
                                           m_CurComputePSO->localWorkgroupSize[2]);
        [encoder setComputePipelineState:m_CurComputePSO->pso];
        [encoder dispatchThreadgroups:size threadsPerThreadgroup:threadgroupSize];

        [encoder endEncoding];
        [m_Cmd encodeSignalEvent:m_RootSignaturesRingSync[m_CurrentRingRootSignatureIndex]
                           value:m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex]];
    }

    void MetalCommandList::Draw() noexcept {}

    void MetalCommandList::SetComputePSO(ComputePSO* pso) noexcept {
        auto* metalPSO = INTERPRET_AS<MetalComputePSO*>(pso);
        m_CurComputePSO = metalPSO;
    }

    void MetalCommandList::SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept {
        m_CBVSRVUAVHeap = INTERPRET_AS<MetalDescriptorHeap*>(CBVSRVUAVHeap);
        m_CBVSRVUAVHeapOffset = 0;
        if (samplerHeap) {
            m_SamplerHeap = INTERPRET_AS<MetalDescriptorHeap*>(samplerHeap);
            m_SamplerHeapOffset = 0;
        }

        RootSignatureT rootSignatureContent{};
        for (size_t spaceIdx = 0; spaceIdx < m_CurRootSignature->spaceDescs.size(); ++spaceIdx) {
            if (m_CurRootSignature->spaceDescs[spaceIdx].rangeDescs[0].rangeType == DescriptorRangeType::Sampler) {
                rootSignatureContent.records[spaceIdx] = m_SamplerHeap->GetHeapBuffer().gpuAddress;
            } else {
                rootSignatureContent.records[spaceIdx] = m_CBVSRVUAVHeap->GetHeapBuffer().gpuAddress;
            }
        }

        if (m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex] != 0) {
            if (++m_CurrentRingRootSignatureIndex > ROOT_SIGNATURE_RING_SIZE) {
                m_CurrentRingRootSignatureIndex = 0;
            }
        }
        WaitForMTLSharedEventValue(m_RootSignaturesRingSync[m_CurrentRingRootSignatureIndex],
                                   m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex],
                                   ~0ul);
        m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex] = 0;
        [m_RootSignaturesRingSync[m_CurrentRingRootSignatureIndex] setSignaledValue:0];

        auto* rootSignaturesRingMem = static_cast<RootSignatureT*>(m_RootSignaturesRing.contents);
        memcpy(rootSignaturesRingMem + m_CurrentRingRootSignatureIndex, &rootSignatureContent, sizeof(rootSignatureContent));
        NSRange range{};
        range.location = m_CurrentRingRootSignatureIndex * sizeof(RootSignatureT);
        range.length = sizeof(RootSignatureT);
        [m_RootSignaturesRing didModifyRange:range];
    }

    void MetalCommandList::CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept {
        auto srcBuffer = INTERPRET_AS<MetalBuffer*>(src);
        auto dstBuffer = INTERPRET_AS<MetalBuffer*>(dst);
        id<MTLBlitCommandEncoder> encoder = [m_Cmd blitCommandEncoder];

        [encoder copyFromBuffer:srcBuffer->buffer sourceOffset:srcOffset toBuffer:dstBuffer->buffer destinationOffset:dstOffset size:size];
        [encoder endEncoding];
    }
    BLAS* MetalCommandList::BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                      const char* name) noexcept {
        auto* result = new MetalBLAS{};
        auto* metalScratch = INTERPRET_AS<MetalBuffer*>(scratchBuffer);

        auto* metalVertex = INTERPRET_AS<MetalBuffer*>(desc.vertexBuffer);
        auto* metalIndex = INTERPRET_AS<MetalBuffer*>(desc.indexBuffer);
        auto* metalTransform = INTERPRET_AS<MetalBuffer*>(desc.transformBuffer);

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
        auto* metalScratch = INTERPRET_AS<MetalBuffer*>(scratchBuffer);

        id<MTLBuffer> instanceDescBuf = [m_Device newBufferWithLength:0 options:MTLResourceOptionCPUCacheModeDefault];

        auto asDescs = [NSMutableArray array];
        for (size_t i = 0; i < desc.blasInstancesCount; ++i) {
            const BLASInstanceDesc& instance = desc.blasInstances[i];
            auto* metalBLAS = INTERPRET_AS<MetalBLAS*>(instance.blas);
            [asDescs addObject:metalBLAS->accelerationStructure];
        }

        auto accelerationStructureDescriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];
        accelerationStructureDescriptor.instanceCount = desc.blasInstancesCount;
        accelerationStructureDescriptor.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeDefault;
        accelerationStructureDescriptor.instancedAccelerationStructures = asDescs;

        accelerationStructureDescriptor.instanceDescriptorBuffer = instanceDescBuf;
        accelerationStructureDescriptor.instanceDescriptorBufferOffset = 0;
        accelerationStructureDescriptor.instanceDescriptorStride = 0;

        // TODO: apply transform from desc

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
        // m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex] += 1;
        // [encoder signal:m_RootSignaturesRingSync[m_CurrentRingRootSignatureIndex];
        // TODO: implement
    }

    void MetalCommandList::BuildRTPSO(RTPSO* pso) noexcept {
        // TODO: implement
    }

    void MetalCommandList::ResourceBarrier(const ResourceBarrierDesc& desc) noexcept {
        //NOOP
    }
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
