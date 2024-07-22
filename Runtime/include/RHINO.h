#pragma once

#include "RHINOTypes.h"

namespace RHINO {
    enum class BackendAPI {
        D3D12,
        Vulkan,
        Metal
    };

    class RHINOInterface {
    public:
        RHINOInterface() = default;
        RHINOInterface(const RHINOInterface&) = delete;
        RHINOInterface(const RHINOInterface&&) = delete;
        virtual ~RHINOInterface() noexcept = default;

    public:
        virtual void Initialize() noexcept = 0;
        virtual void Release() noexcept = 0;

    public:
        // PSO MANAGEMENT
        virtual RootSignature* SerializeRootSignature(const RootSignatureDesc& desc) noexcept = 0;

        virtual RTPSO* CreateRTPSO(const RTPSODesc& desc) noexcept = 0;
        virtual RTPSO* CreateSCARRTPSO(const void* scar, uint32_t sizeInBytes, const RTPSODesc& desc) noexcept = 0;
        virtual ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept = 0;
        virtual ComputePSO* CompileSCARComputePSO(const void* scar, uint32_t sizeInBytes, RootSignature* rootSignature,
                                                  const char* debugName) noexcept = 0;

    public:
        // RESOURCE MANAGEMENT
        virtual Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride,
                                     const char* name) noexcept = 0;
        virtual void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept = 0;
        virtual void UnmapMemory(Buffer* buffer) noexcept = 0;

        virtual Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                           ResourceUsage usage, const char* name) noexcept = 0;
        virtual Sampler* CreateSampler(const SamplerDesc& desc) noexcept = 0;
        virtual DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept = 0;
        virtual Swapchain* CreateSwapchain(const SwapchainDesc& desc) noexcept = 0;

        //TODO: add command list target queue setting;
        virtual CommandList* AllocateCommandList(const char* name) noexcept = 0;

    public:
        virtual ASPrebuildInfo GetBLASPrebuildInfo(const BLASDesc& desc) noexcept = 0;
        virtual ASPrebuildInfo GetTLASPrebuildInfo(const TLASDesc& desc) noexcept = 0;

    public:
        // JOB SUBMISSION
        virtual void SubmitCommandList(CommandList* cmd) noexcept = 0;
        virtual void SwapchainPresent(Swapchain* swapchain, Texture2D* toPresent, size_t width, size_t height) noexcept = 0;

    public:
        // Sync API
        virtual Semaphore* CreateSyncSemaphore(uint64_t initialValue) noexcept = 0;

        virtual void SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept = 0;
        virtual void SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept = 0;
        virtual bool SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept = 0;
        virtual void SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept = 0;
        virtual uint64_t GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept = 0;
    };

    RHINOInterface* CreateRHINO(BackendAPI backendApi) noexcept;
}// namespace RHINO
