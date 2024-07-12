#pragma once

#import <Metal/Metal.h>
#include "MetalBackendTypes.h"
#include "MetalDescriptorHeap.h"


namespace RHINO::APIMetal {
    class MetalCommandList final : public CommandList {
    private:
        id<MTLCommandBuffer> m_Cmd = nil;
        id<MTLDevice> m_Device = nil;

        MetalComputePSO* m_CurComputePSO = nullptr;
        MetalDescriptorHeap* m_CBVSRVUAVHeap = nullptr;
        size_t m_CBVSRVUAVHeapOffset = 0;
        MetalDescriptorHeap* m_SamplerHeap = nullptr;
        size_t m_SamplerHeapOffset = 0;
    public:
        void Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue) noexcept;
        void SubmitToQueue() noexcept;
        void Release() noexcept;
    public:
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void Draw() noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept final;
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void DispatchRays(const DispatchRaysDesc& desc) noexcept final;

    public:
        BLAS* BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;
        TLAS* BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;
        void BuildRTPSO(RTPSO* pso) noexcept final;
    };
} // namespace RHINO::APIMetal
