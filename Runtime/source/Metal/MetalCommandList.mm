#include "MetalCommandList.h"
#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

namespace RHINO::APIMetal {
    void MetalCommandList::Dispatch(const DispatchDesc& desc) noexcept {
        id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];

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

    void MetalCommandList::SetRTPSO(RTPSO* pso) noexcept {}

    void MetalCommandList::SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept {
        auto* metalCBVSRVUAVHeap = static_cast<MetalDescriptorHeap*>(CBVSRVUAVHeap);
        auto* metalSamplerHeap = static_cast<MetalDescriptorHeap*>(CBVSRVUAVHeap);

        id<MTLComputeCommandEncoder> encoder = [cmd computeCommandEncoder];
        if (samplerHeap) {
            // [encoder setBuffer: ]
        }
        else {
            [encoder setBuffer:metalCBVSRVUAVHeap->m_ArgBuf offset:0 atIndex:kIRArgumentBufferBindPoint];
        }
    }
} // namespace RHINO::APIMetal
