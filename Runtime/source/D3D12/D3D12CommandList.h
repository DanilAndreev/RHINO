#pragma once

#ifdef ENABLE_API_D3D12

#include "D3D12GarbageCollector.h"
#include "RHINOTypes.h"

namespace RHINO::APID3D12 {
    class D3D12CommandList : public CommandList {
    private:
        ID3D12Device5* m_Device = nullptr;
        ID3D12CommandAllocator* m_Allocator = nullptr;
        ID3D12GraphicsCommandList4* m_Cmd = nullptr;
        ID3D12Fence* m_Fence = nullptr;
        size_t m_FenceNextVal = 0;

        D3D12GarbageCollector* m_GarbageCollector = nullptr;

    public:
        void Initialize(const char* name, ID3D12Device5* device, D3D12GarbageCollector* garbageCollector) noexcept;
        void SumbitToQueue(ID3D12CommandQueue* queue) noexcept;

    public:
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept final;
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void DispatchRays(const DispatchRaysDesc& desc) noexcept final;
        void Draw() noexcept final;
        void ResourceBarrier(const ResourceBarrierDesc& desc) noexcept final;

    public:
        void Release() noexcept final;

    public:
        void BuildRTPSO(RTPSO* pso) noexcept final;
        BLAS* BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;
        TLAS* BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;

    private:
        ID3D12Resource* CreateStagingBuffer(size_t size, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_STATES initialState) noexcept;
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
