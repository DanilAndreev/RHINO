#pragma once

#import <Metal/Metal.h>

namespace RHINO::APIMetal {
    class MetalCommandList : public CommandList {
    public:
        id<MTLCommandBuffer> cmd = nil;

    public:
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void Draw() noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetRTPSO(RTPSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* heap) noexcept final;
    };
}// namespace RHINO::APIMetal
