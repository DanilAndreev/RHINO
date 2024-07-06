#pragma once

#ifdef ENABLE_API_D3D12

#include "RHINOTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Backend;

    class D3D12CommandList : public CommandList {
    public:
        ID3D12Device5* device = nullptr;
        ID3D12CommandAllocator* allocator = nullptr;
        ID3D12GraphicsCommandList4* cmd = nullptr;
    public:
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetRTPSO(RTPSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept final;
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void DispatchRays() noexcept;
        void Draw() noexcept final;

    public:
        BLAS* BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept;
        TLAS* BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept;

    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
