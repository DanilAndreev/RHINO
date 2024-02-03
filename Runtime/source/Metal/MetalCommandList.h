#pragma once

#import <Metal/Metal.h>
#include "MetalBackendTypes.h"
#include "MetalDescriptorHeap.h"


namespace RHINO::APIMetal {
    class MetalCommandList final : public CommandList {
    public:
        id<MTLCommandBuffer> cmd = nil;

        MetalComputePSO* m_CurComputePSO = nullptr;
        MetalDescriptorHeap* m_CBVSRVUAVHeap = nullptr;
        size_t m_CBVSRVUAVHeapOffset = 0;
        MetalDescriptorHeap* m_SamplerHeap = nullptr;
        size_t m_SamplerHeapOffset = 0;
    public:
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void Draw() noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetRTPSO(RTPSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept final;
    };
} // namespace RHINO::APIMetal
