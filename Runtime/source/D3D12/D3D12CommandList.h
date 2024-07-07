#pragma once

#ifdef ENABLE_API_D3D12

#include "RHINOTypes.h"

namespace RHINO::APID3D12 {
    class D3D12CommandList : public CommandList {
    public:
        ID3D12Device5* m_Device = nullptr;
        ID3D12CommandAllocator* m_Allocator = nullptr;
        ID3D12GraphicsCommandList4* m_Cmd = nullptr;
        ID3D12Fence* m_Fence;
        size_t m_FenceNextVal = 0;

    public:
        void Initialize(const char* name, ID3D12Device5* device) noexcept;
        void Release() noexcept;
        void SumbitToQueue(ID3D12CommandQueue* queue) noexcept;

    public:
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetRTPSO(RTPSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept final;
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void DispatchRays(const DispatchRaysDesc& desc) noexcept;
        void Draw() noexcept final;

    public:
        void BuildRTPSO(RTPSO* pso) noexcept;
        BLAS* BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept;
        TLAS* BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept;
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
