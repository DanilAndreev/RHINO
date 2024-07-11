#pragma once

#ifdef ENABLE_API_D3D12

#include "RHINOInterfaceImplBase.h"
#include "D3D12BackendTypes.h"
#include "D3D12GarbageCollector.h"

namespace RHINO::APID3D12 {
    class D3D12Backend : public RHINOInterfaceImplBase {
    public:
        explicit D3D12Backend() noexcept;

    public:
        void Initialize() noexcept final;
        void Release() noexcept final;

    public:
        RTPSO* CreateRTPSO(const RTPSODesc& desc) noexcept final;
        void ReleaseRTPSO(RTPSO* pso) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;
        void ReleaseComputePSO(ComputePSO* pso) noexcept final;

    public:
        Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void ReleaseBuffer(Buffer* buffer) noexcept final;
        void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept final;
        void UnmapMemory(Buffer* buffer) noexcept final;

        Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                   ResourceUsage usage, const char* name) noexcept final;
        void ReleaseTexture2D(Texture2D* texture) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType heapType, size_t descriptorsCount, const char* name) noexcept final;
        void ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept final;
        CommandList* AllocateCommandList(const char* name) noexcept final;
        void ReleaseCommandList(CommandList* commandList) noexcept final;
        Semaphore* CreateSyncSemaphore(uint64_t initialValue) noexcept final;
    public:
        ASPrebuildInfo GetBLASPrebuildInfo(const BLASDesc& desc) noexcept final;
        ASPrebuildInfo GetTLASPrebuildInfo(const TLASDesc& desc) noexcept final;

    public:
        void SubmitCommandList(CommandList* cmd) noexcept final;
        void QueueSignal(Semaphore* semaphore, uint64_t value) noexcept final;
        bool WaitForSemaphore(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept final;

    private:
        ID3D12RootSignature* CreateRootSignature(size_t spacesCount, const DescriptorSpaceDesc* spaces) noexcept;

    private:
        ID3D12Device5* m_Device = nullptr;
        ID3D12CommandQueue* m_DefaultQueue = nullptr;
        ID3D12Fence* m_DefaultQueueFence = nullptr;
        UINT64 m_DefaultQueueFenceLastVal = 0;
        ID3D12CommandQueue* m_ComputeQueue = nullptr;
        ID3D12Fence* m_ComputeQueueFence = nullptr;
        UINT64 m_ComputeQueueFenceLastVal = 0;
        ID3D12CommandQueue* m_CopyQueue = nullptr;
        ID3D12Fence* m_CopyQueueFence = nullptr;
        UINT64 m_CopyQueueFenceLastVal = 0;

        D3D12GarbageCollector m_GarbageCollector = {};
    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
