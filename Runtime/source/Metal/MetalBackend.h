#pragma once

#ifdef ENABLE_API_METAL

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include "RHINOInterfaceImplBase.h"
#include "MetalBackendTypes.h"

namespace RHINO::APIMetal {
    class MetalTexture2D;
    class MetalBackend : public RHINOInterfaceImplBase {
    public:
        MetalBackend() noexcept {}

    public:
        void Initialize() noexcept final;
        void Release() noexcept final;

    public:
        RootSignature* SerializeRootSignature(const RHINO::RootSignatureDesc &desc) noexcept final;
        RTPSO* CreateRTPSO(const RTPSODesc& desc) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;

    public:
        Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept final;
        void UnmapMemory(Buffer* buffer) noexcept final;

        Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format, ResourceUsage usage,
                                   const char* name) noexcept final;
        Sampler* CreateSampler(const RHINO::SamplerDesc &desc) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept final;
        CommandList* AllocateCommandList(const char* name) noexcept final;

    public:
        // JOB SUBMISSION
        void SubmitCommandList(CommandList* cmd) noexcept final;
        void SwapchainPresent(Swapchain *swapchain, Texture2D *toPresent, size_t width, size_t height) noexcept final;
        ASPrebuildInfo GetBLASPrebuildInfo(const BLASDesc& desc) noexcept final;
        ASPrebuildInfo GetTLASPrebuildInfo(const TLASDesc& desc) noexcept final;

    public:
        Semaphore* CreateSyncSemaphore(uint64_t initialValue) noexcept final;
        void SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept final;
        void SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept final;
        bool SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept final;
        void SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept final;
        uint64_t GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept final;

    private:
        id<MTLDevice> m_Device = nil;
        id<MTLCommandQueue> m_DefaultQueue;
        id<MTLCommandQueue> m_AsyncComputeQueue;
        id<MTLCommandQueue> m_CopyQueue;

        IRCompiler* m_IRCompiler = nullptr;
    };

    RHINOInterface* AllocateMetalBackend() noexcept {
        return new MetalBackend{};
    }
}// namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
