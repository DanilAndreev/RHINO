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
        virtual RTPSO* CompileRTPSO(const RTPSODesc& desc) noexcept = 0;
        virtual void ReleaseRTPSO(RTPSO* pso) noexcept = 0;
        virtual ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept = 0;
        virtual void ReleaseComputePSO(ComputePSO* pso) noexcept = 0;

    public:
        // RESOURCE MANAGEMENT
        virtual Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept = 0;
        virtual void ReleaseBuffer(Buffer* buffer) noexcept = 0;
        virtual void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept = 0;
        virtual void UnmapMemory(Buffer* buffer) noexcept = 0;

        virtual Texture2D* CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                           ResourceUsage usage, const char* name) noexcept = 0;
        virtual void ReleaseTexture2D(Texture2D* texture) noexcept = 0;

        virtual DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept = 0;
        virtual void ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept = 0;

        virtual CommandList* AllocateCommandList(const char* name) noexcept = 0;
        virtual void ReleaseCommandList(CommandList* commandList) noexcept = 0;

    public:
        // JOB SUBMISSION
        virtual void SubmitCommandList(CommandList* cmd) noexcept = 0;
    };

    RHINOInterface* CreateRHINO(BackendAPI backendApi) noexcept;
}// namespace RHINO
