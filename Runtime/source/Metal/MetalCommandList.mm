#ifdef ENABLE_API_METAL

#include "MetalCommandList.h"

#import <metal_irconverter_runtime/metal_irconverter_runtime.h>

#include "MetalConverters.h"
#include "MetalUtils.h"


namespace RHINO::APIMetal {
    struct IndirectResourcesSet {
        std::vector<id<MTLResource>> r;
        std::vector<id<MTLResource>> rw;
        static constexpr MTLResourceUsage rUsage = MTLResourceUsageRead | MTLResourceUsageSample;
        static constexpr MTLResourceUsage rwUsage = MTLResourceUsageRead | MTLResourceUsageWrite | MTLResourceUsageSample;
    };

    static IndirectResourcesSet GatherIndirectResources(MetalDescriptorHeap* CBVSRVUAVHeap, size_t CBVSRVUAVHeapOffset,
                                                        MetalRootSignature* rootSignature) {
        IndirectResourcesSet result{};
        for (const DescriptorSpaceDesc& space: rootSignature->spaceDescs) {
            for (size_t spaceIdx = 0; spaceIdx < space.rangeDescCount; ++spaceIdx) {
                size_t pos = space.rangeDescs[spaceIdx].baseRegisterSlot + space.offsetInDescriptorsFromTableStart;
                switch (space.rangeDescs[spaceIdx].rangeType) {
                    case DescriptorRangeType::CBV:
                    case DescriptorRangeType::SRV: {
                        for (size_t i = 0; i < space.rangeDescs[spaceIdx].descriptorsCount; ++i) {
                            const auto& resources = CBVSRVUAVHeap->GetBoundResources();
                            const auto& resEntry = resources[CBVSRVUAVHeapOffset + pos + i];
                            if (resEntry.direct != nil) {
                                result.r.push_back(resEntry.direct);
                                for (const id<MTLResource>& indirectResource : resEntry.indirect) {
                                    if (indirectResource != nil) {
                                        result.r.push_back(indirectResource);
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case DescriptorRangeType::UAV: {
                        for (size_t i = 0; i < space.rangeDescs[spaceIdx].descriptorsCount; ++i) {
                            const auto& resources = CBVSRVUAVHeap->GetBoundResources();
                            const auto& resEntry = resources[CBVSRVUAVHeapOffset + pos + i];
                            if (resEntry.direct != nil) {
                                result.rw.push_back(resEntry.direct);
                                for (const id<MTLResource>& indirectResource : resEntry.indirect) {
                                    if (indirectResource != nil) {
                                        result.rw.push_back(indirectResource);
                                    }
                                }
                            }
                        }
                        break;
                    }
                    default:
                        // Skipping SMP
                        break;
                }
            }
        }
        return result;
    }


    void MetalCommandList::Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue) noexcept {
        m_Device = device;
        m_RootSignaturesRing = [m_Device newBufferWithLength:sizeof(RootSignatureT) * ROOT_SIGNATURE_RING_SIZE
                                                     options:MTLResourceStorageModeManaged];
        [m_RootSignaturesRing setLabel: @"RHINO::CommandList::RootSignatureRing"];
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

        IndirectResourcesSet indirectRes = GatherIndirectResources(m_CBVSRVUAVHeap, m_CBVSRVUAVHeapOffset, m_CurRootSignature);

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

        [encoder useResources:indirectRes.r.data() count:indirectRes.r.size() usage:indirectRes.rUsage];
        [encoder useResources:indirectRes.rw.data() count:indirectRes.rw.size() usage:indirectRes.rwUsage];

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
        m_SamplerHeap = samplerHeap ? INTERPRET_AS<MetalDescriptorHeap*>(samplerHeap) : nullptr;
        m_CBVSRVUAVHeapOffset = 0;
        m_SamplerHeapOffset = 0;
        SetHeapHelper(m_CBVSRVUAVHeap, m_CBVSRVUAVHeapOffset, m_SamplerHeap, m_SamplerHeapOffset);
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
        auto* metalTransform = desc.transformBuffer ? INTERPRET_AS<MetalBuffer*>(desc.transformBuffer) : nullptr;

        auto triangleGeoDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
        triangleGeoDesc.vertexBuffer = metalVertex->buffer;
        triangleGeoDesc.vertexBufferOffset = desc.vertexBufferStartOffset;
        triangleGeoDesc.vertexFormat = Convert::ToMTLMTLAttributeFormat(desc.vertexFormat);
        triangleGeoDesc.vertexStride = desc.vertexStride;
        triangleGeoDesc.indexBuffer = metalIndex->buffer;
        triangleGeoDesc.indexBufferOffset = desc.indexBufferStartOffset;
        triangleGeoDesc.indexType = MTLIndexTypeUInt16;
        triangleGeoDesc.triangleCount = desc.indexCount / 3;
        triangleGeoDesc.primitiveDataBuffer = nil;
        triangleGeoDesc.primitiveDataStride = 0;
        triangleGeoDesc.primitiveDataElementSize = 0;
        triangleGeoDesc.transformationMatrixBuffer = desc.transformBuffer ? metalTransform->buffer : nil;
        triangleGeoDesc.transformationMatrixBufferOffset = desc.transformBuffer ? desc.transformBufferStartOffset : 0;
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

        const size_t instanceDescBufSize = sizeof(MTLAccelerationStructureInstanceDescriptor) * desc.blasInstancesCount;
        id<MTLBuffer> instanceDescBuf = [m_Device newBufferWithLength:instanceDescBufSize
                                                              options:MTLResourceStorageModeShared];
        [instanceDescBuf setLabel: @"RHINO::BuildTLAS::InstanceDescBuffer"];

        auto asDescs = [NSMutableArray array];

        auto* instanceDescBufContents = static_cast<MTLAccelerationStructureInstanceDescriptor*>(instanceDescBuf.contents);
        std::vector<uint32_t> instanceContribution{};
        instanceContribution.reserve(desc.blasInstancesCount);
        for (size_t i = 0; i < desc.blasInstancesCount; ++i) {
            const BLASInstanceDesc& instance = desc.blasInstances[i];
            auto* metalBLAS = INTERPRET_AS<MetalBLAS*>(instance.blas);
            [asDescs addObject:metalBLAS->accelerationStructure];
            result->indirectResources.emplace_back(metalBLAS->accelerationStructure);
            instanceContribution.emplace_back(instance.instanceID);

            const auto& t = instance.transform;
            MTLPackedFloat4x3 transform{MTLPackedFloat3Make(t[0][0], t[1][0], t[2][0]),
                                        MTLPackedFloat3Make(t[0][1], t[1][1], t[2][1]),
                                        MTLPackedFloat3Make(t[0][2], t[1][2], t[2][2]),
                                        MTLPackedFloat3Make(t[0][3], t[1][3], t[2][3])};

            instanceDescBufContents[i].accelerationStructureIndex = instance.instanceID;
            instanceDescBufContents[i].mask = instance.instanceMask;
            instanceDescBufContents[i].transformationMatrix = transform;
            instanceDescBufContents[i].options = MTLAccelerationStructureInstanceOptionNone;
            //TODO: calculate and fill
            instanceDescBufContents[i].intersectionFunctionTableOffset = 0;
        }
        // [instanceDescBuf didModifyRange:NSMakeRange(0, sizeof(instanceDescBufSize))];

        const size_t gpuASHeaderSize = sizeof(IRRaytracingAccelerationStructureGPUHeader) + instanceContribution.size() * sizeof(uint32_t);
        result->gpuASHeader = [m_Device newBufferWithLength:gpuASHeaderSize options:0];
        if(name) {
            std::string debugName = std::string{name} + ".GPUHeader";
            [result->gpuASHeader setLabel: [NSString stringWithUTF8String:debugName.c_str()]];
        }
        auto ASHeader = static_cast<IRRaytracingAccelerationStructureGPUHeader*>([result->gpuASHeader contents]);
        auto ASHeaderInstanceContribution = reinterpret_cast<uint32_t*>(&ASHeader[1]);
        ASHeader->addressOfInstanceContributions = [result->gpuASHeader gpuAddress] + sizeof(IRRaytracingAccelerationStructureGPUHeader);
        IRRaytracingSetAccelerationStructure(reinterpret_cast<uint8_t*>(ASHeader),
                                             [result->accelerationStructure gpuResourceID],
                                             reinterpret_cast<uint8_t*>(ASHeaderInstanceContribution),
                                             instanceContribution.data(), instanceContribution.size());

        auto accelerationStructureDescriptor = [MTLInstanceAccelerationStructureDescriptor descriptor];
        accelerationStructureDescriptor.instanceCount = desc.blasInstancesCount;
        accelerationStructureDescriptor.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeDefault;
        accelerationStructureDescriptor.instancedAccelerationStructures = asDescs;

        accelerationStructureDescriptor.instanceDescriptorBuffer = instanceDescBuf;
        accelerationStructureDescriptor.instanceDescriptorBufferOffset = 0;
        accelerationStructureDescriptor.instanceDescriptorStride = sizeof(MTLAccelerationStructureInstanceDescriptor);

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
        auto* metalPSO = INTERPRET_AS<MetalRTPSO*>(desc.pso);
        auto CBVSRVUAVHeap = INTERPRET_AS<MetalDescriptorHeap*>(desc.CDBSRVUAVHeap);
        MetalDescriptorHeap* samplerHeap = desc.samplerHeap ? INTERPRET_AS<MetalDescriptorHeap*>(desc.samplerHeap) : nullptr;
        size_t CBVSRVUAVHeapOffset = 0;
        size_t samplerHeapOffset = 0;

        SetHeapHelper(CBVSRVUAVHeap, CBVSRVUAVHeapOffset, samplerHeap, samplerHeapOffset);

        id<MTLComputeCommandEncoder> encoder = [m_Cmd computeCommandEncoder];


        [encoder useResource:CBVSRVUAVHeap->GetHeapBuffer() usage:MTLResourceUsageRead];
        if (samplerHeap) {
            [encoder useResource:samplerHeap->GetHeapBuffer() usage:MTLResourceUsageRead];
        }

        IndirectResourcesSet indirectRes = GatherIndirectResources(CBVSRVUAVHeap, CBVSRVUAVHeapOffset, m_CurRootSignature);
        [encoder useResources:indirectRes.r.data() count:indirectRes.r.size() usage:indirectRes.rUsage];
        [encoder useResources:indirectRes.rw.data() count:indirectRes.rw.size() usage:indirectRes.rwUsage];

        [encoder useResource:metalPSO->vft usage:MTLResourceUsageRead];
        [encoder useResource:metalPSO->ift usage:MTLResourceUsageRead];
        [encoder useResource:metalPSO->shaderTable usage:MTLResourceUsageRead];
        [encoder useResource:m_RootSignaturesRing usage:MTLResourceUsageRead];

        const size_t rootSignatureOffset = m_CurrentRingRootSignatureIndex * sizeof(RootSignatureT);
        m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex] += 1;

        const size_t recordStride = metalPSO->shaderTableRecordStride;
        IRDispatchRaysDescriptor dispatchRaysDesc;

        dispatchRaysDesc.RayGenerationShaderRecord = {
                .StartAddress = [metalPSO->shaderTable gpuAddress] + recordStride * desc.rayGenerationShaderRecordIndex,
                //TODO: SizeInBytes is size of table but not one entry.
                .SizeInBytes = sizeof(IRShaderIdentifier)
        };
        dispatchRaysDesc.HitGroupTable = {
                .StartAddress = [metalPSO->shaderTable gpuAddress] + recordStride * desc.hitGroupStartRecordIndex,
                .SizeInBytes = sizeof(IRShaderIdentifier),
                .StrideInBytes = recordStride,
        };
        dispatchRaysDesc.MissShaderTable = {
                .StartAddress = [metalPSO->shaderTable gpuAddress] + recordStride * desc.hitGroupStartRecordIndex,
                .SizeInBytes = sizeof(IRShaderIdentifier),
                .StrideInBytes = recordStride,
        };
        dispatchRaysDesc.CallableShaderTable = {
                .StartAddress = 0,
                .SizeInBytes = 0,
                .StrideInBytes = 0
        };
        dispatchRaysDesc.Width = desc.width;
        dispatchRaysDesc.Height = desc.height;
        dispatchRaysDesc.Depth = 1;

        IRDispatchRaysArgument dispatchRaysArgs;
        dispatchRaysArgs.DispatchRaysDesc          = dispatchRaysDesc;
        dispatchRaysArgs.GRS                       = [m_RootSignaturesRing gpuAddress] + rootSignatureOffset;
        // Heap offsets are taken in account by root signature in SetHeapHelper
        dispatchRaysArgs.ResDescHeap               = [CBVSRVUAVHeap->GetHeapBuffer() gpuAddress];
        dispatchRaysArgs.SmpDescHeap               = samplerHeap ? [samplerHeap->GetHeapBuffer() gpuAddress] : 0;
        dispatchRaysArgs.VisibleFunctionTable      = [metalPSO->vft gpuResourceID];
        dispatchRaysArgs.IntersectionFunctionTable = [metalPSO->ift gpuResourceID];

        [encoder setBytes:&dispatchRaysArgs
                   length:sizeof(dispatchRaysArgs)
                  atIndex:kIRRayDispatchArgumentsBindPoint];

        auto size = MTLSizeMake(desc.width, desc.height, 1);
        auto threadgroupSize = MTLSizeMake([metalPSO->pso maxTotalThreadsPerThreadgroup], 1, 1);
        [encoder setComputePipelineState:metalPSO->pso];
        [encoder dispatchThreadgroups:size threadsPerThreadgroup:threadgroupSize];

        [encoder endEncoding];
        [m_Cmd encodeSignalEvent:m_RootSignaturesRingSync[m_CurrentRingRootSignatureIndex]
                           value:m_RootSignaturesRingSyncWaitValue[m_CurrentRingRootSignatureIndex]];
    }

    void MetalCommandList::BuildRTPSO(RTPSO* pso) noexcept {
        // TODO: implement
    }

    void MetalCommandList::ResourceBarrier(const ResourceBarrierDesc& desc) noexcept {
        //NOOP
    }

    void MetalCommandList::SetHeapHelper(MetalDescriptorHeap* CBVSRVUAVHeap, size_t CBVSRVUAVHeapOffset, MetalDescriptorHeap* samplerHeap,
                                         size_t samplerHeapOffset) noexcept {
        RootSignatureT rootSignatureContent{};
        for (size_t spaceIdx = 0; spaceIdx < m_CurRootSignature->spaceDescs.size(); ++spaceIdx) {
            const auto& spaceDesc = m_CurRootSignature->spaceDescs[spaceIdx];
            RootSignatureRecordT record = 0;
            if (spaceDesc.rangeDescs[0].rangeType == DescriptorRangeType::Sampler) {
                record = [samplerHeap->GetHeapBuffer() gpuAddress] + samplerHeapOffset;
                record += spaceDesc.offsetInDescriptorsFromTableStart * samplerHeap->GetDescriptorStride();
            } else {
                record = [CBVSRVUAVHeap->GetHeapBuffer() gpuAddress] + CBVSRVUAVHeapOffset;
                record += spaceDesc.offsetInDescriptorsFromTableStart * CBVSRVUAVHeap->GetDescriptorStride();
            }
            rootSignatureContent.records[spaceIdx] = record;
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
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
