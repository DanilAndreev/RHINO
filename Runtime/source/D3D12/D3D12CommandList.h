#pragma once

#ifdef ENABLE_API_D3D12

namespace RHINO::APID3D12 {
    class D3D12CommandList : public CommandList {
    public:
        ID3D12CommandAllocator* allocator = nullptr;
        ID3D12GraphicsCommandList* cmd = nullptr;
    public:
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetRTPSO(RTPSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept final;
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void Draw() noexcept final;

    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
