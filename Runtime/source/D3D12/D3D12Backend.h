#pragma once

#ifdef ENABLE_API_D3D12

#include <dxgi.h>

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
        RootSignature* SerializeRootSignature(const RootSignatureDesc& desc) noexcept final;
        RTPSO* CreateRTPSO(const RTPSODesc& desc) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;

    public:
        Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept final;
        void UnmapMemory(Buffer* buffer) noexcept final;

        Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                   ResourceUsage usage, const char* name) noexcept final;
        Sampler* CreateSampler(const SamplerDesc& desc) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType heapType, size_t descriptorsCount, const char* name) noexcept final;
        Swapchain* CreateSwapchain(const SwapchainDesc& desc) noexcept final;

        CommandList* AllocateCommandList(const char* name) noexcept final;
    public:
        ASPrebuildInfo GetBLASPrebuildInfo(const BLASDesc& desc) noexcept final;
        ASPrebuildInfo GetTLASPrebuildInfo(const TLASDesc& desc) noexcept final;

    public:
        void SubmitCommandList(CommandList* cmd) noexcept final;

    public:
        Semaphore* CreateSyncSemaphore(uint64_t initialValue) noexcept final;

        void SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept final;
        void SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept final;
        bool SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept final;
        void SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept final;
        uint64_t GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept final;

    private:
        ID3D12RootSignature* CreateRootSignature(size_t spacesCount, const DescriptorSpaceDesc* spaces) noexcept;

    private:
        IDXGIFactory* m_DXGIFactory = nullptr;
        ID3D12Device5* m_Device = nullptr;
        ID3D12CommandQueue* m_DefaultQueue = nullptr;
        ID3D12CommandQueue* m_ComputeQueue = nullptr;
        ID3D12CommandQueue* m_CopyQueue = nullptr;

        D3D12GarbageCollector m_GarbageCollector = {};
    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
