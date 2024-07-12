#pragma once

#ifdef ENABLE_API_METAL

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include "RHINOInterfaceImplBase.h"

namespace RHINO::APIMetal {
    class MetalTexture2D;
    class MetalBackend : public RHINOInterfaceImplBase {
    private:
        id<MTLDevice> m_Device = nil;
        id<MTLCommandQueue> m_DefaultQueue;
        id<MTLCommandQueue> m_AsyncComputeQueue;
        id<MTLCommandQueue> m_CopyQueue;

    public:
        MetalBackend() noexcept {}

    public:
        void Initialize() noexcept final;
        void Release() noexcept final;

    public:
        RTPSO* CreateRTPSO(const RTPSODesc& desc) noexcept final;
        void ReleaseRTPSO(RTPSO* pso) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;
        ComputePSO* CompileSCARComputePSO(const void* scar, uint32_t sizeInBytes,
                                          const char* debugName) noexcept final;
        void ReleaseComputePSO(ComputePSO* pso) noexcept final;

    public:
        Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void ReleaseBuffer(Buffer* buffer) noexcept final;
        void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept final;
        void UnmapMemory(Buffer* buffer) noexcept final;

        Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format, ResourceUsage usage,
                                   const char* name) noexcept final;
        void ReleaseTexture2D(Texture2D* texture) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept final;
        void ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept final;
        CommandList* AllocateCommandList(const char* name) noexcept final;
        void ReleaseCommandList(CommandList* cmd) noexcept final;

    public:
        // JOB SUBMISSION
        void SubmitCommandList(CommandList* cmd) noexcept final;
        ASPrebuildInfo GetBLASPrebuildInfo(const BLASDesc& desc) noexcept final;
        ASPrebuildInfo GetTLASPrebuildInfo(const TLASDesc& desc) noexcept final;

    public:
        Semaphore* CreateSyncSemaphore(uint64_t initialValue) noexcept final;
        void ReleaseSyncSemaphore(Semaphore* semaphore) noexcept final;
        void SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept final;
        void SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept final;
        bool SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept final;
        void SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept final;
        uint64_t GetSemaphoreCompleatedValue(const Semaphore* semaphore) noexcept final;
    };

    RHINOInterface* AllocateMetalBackend() noexcept {
        return new MetalBackend{};
    }
}// namespace RHINO::APIMetal

#endif// ENABLE_API_METAL