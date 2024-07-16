#ifdef ENABLE_API_METAL

#include "MetalCommandList.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

#include "MetalConverters.h"

namespace RHINO::APIMetal {

    void MetalCommandList::Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue) noexcept {
        m_Device = device;
        m_RootSignature = [m_Device newBufferWithLength:MAX_ROOT_SIGNATURE_SIZE_IN_RECORDS * sizeof(RootSignatureRecordT)
                                                options:MTLResourceStorageModeManaged];
        [m_RootSignature setLabel: @"RootSignature"];
        m_Cmd = [queue commandBuffer];
    }

    void MetalCommandList::SubmitToQueue() noexcept { [m_Cmd commit]; }

    void MetalCommandList::Release() noexcept {
        delete this;
    }

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
                        usedCBVSRVs.push_back(m_CBVSRVUAVHeap->m_Resources[m_CBVSRVUAVHeapOffset + pos]);
                        break;
                    case DescriptorRangeType::UAV:
                        usedUAVs.push_back(m_CBVSRVUAVHeap->m_Resources[m_CBVSRVUAVHeapOffset + pos]);
                        break;
                    case DescriptorRangeType::Sampler:
                        if (m_SamplerHeap)
                            usedSMPs.push_back(m_SamplerHeap->m_Resources[m_SamplerHeapOffset + pos]);
                        break;
                }
            }
        }

        // TODO: split PSO to RootSignature and actual PSO and read those data from RS
        {
            RootSignatureRecordT rootSignatureContent[MAX_ROOT_SIGNATURE_SIZE_IN_RECORDS] = {};

            for (size_t spaceID = 0; spaceID < m_CurComputePSO->spaceDescs.size(); ++spaceID) {
                const DescriptorSpaceDesc& space = m_CurComputePSO->spaceDescs[spaceID];

                for (size_t i = 0; i < space.rangeDescCount; ++i) {
                    size_t offInDescriptors = space.offsetInDescriptorsFromTableStart + space.rangeDescs[i].baseRegisterSlot;
                    switch (space.rangeDescs[i].rangeType) {
                        case DescriptorRangeType::CBV:
                        case DescriptorRangeType::SRV:
                        case DescriptorRangeType::UAV: {
                            offInDescriptors += m_CBVSRVUAVHeapOffset;
                            const size_t offInBytes = offInDescriptors * sizeof(rootSignatureContent[0]);
                            rootSignatureContent[0] = m_CBVSRVUAVHeap->GetHeapBuffer().gpuAddress + offInBytes;
                            break;
                        }
                        case DescriptorRangeType::Sampler: {
                            offInDescriptors += m_SamplerHeapOffset;
                            const size_t offInBytes = offInDescriptors * sizeof(rootSignatureContent[0]);
                            if (m_SamplerHeap)
                                rootSignatureContent[0] = m_SamplerHeap->GetHeapBuffer().gpuAddress + offInBytes;
                            break;
                        }
                    }
                }
            }

            assert(m_RootSignature.allocatedSize >= sizeof(rootSignatureContent));
            memcpy(m_RootSignature.contents, rootSignatureContent, sizeof(rootSignatureContent));
            NSRange range{};
            range.location = 0;
            range.length = sizeof(rootSignatureContent);
            [m_RootSignature didModifyRange:range];

        }

        [encoder setBuffer:m_RootSignature offset:0 atIndex:kIRArgumentBufferBindPoint];

        [encoder useResource:m_RootSignature usage:MTLResourceUsageRead];
        [encoder useResource:m_CBVSRVUAVHeap->GetHeapBuffer() usage:MTLResourceUsageRead];
        [encoder useResources:usedUAVs.data() count:usedUAVs.size() usage:MTLResourceUsageRead | MTLResourceUsageWrite];
        [encoder useResources:usedCBVSRVs.data() count:usedCBVSRVs.size() usage:MTLResourceUsageRead | MTLResourceUsageSample];

        auto size = MTLSizeMake(desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
        // TODO: validate correctness
        auto threadgroupSize = MTLSizeMake(m_CurComputePSO->pso.maxTotalThreadsPerThreadgroup, 1, 1);
        [encoder setComputePipelineState:m_CurComputePSO->pso];
        [encoder dispatchThreadgroups:size threadsPerThreadgroup:threadgroupSize];
        [encoder endEncoding];
    }

    void MetalCommandList::Draw() noexcept {}

    void MetalCommandList::SetComputePSO(ComputePSO* pso) noexcept {
        auto* metalPSO = INTERPRET_AS<MetalComputePSO*>(pso);
        m_CurComputePSO = metalPSO;
    }

    void MetalCommandList::SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept {
        m_CBVSRVUAVHeap = INTERPRET_AS<MetalDescriptorHeap*>(CBVSRVUAVHeap);
        m_CBVSRVUAVHeapOffset = 0;
        m_SamplerHeap = INTERPRET_AS<MetalDescriptorHeap*>(CBVSRVUAVHeap);
        m_SamplerHeapOffset = 0;
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
