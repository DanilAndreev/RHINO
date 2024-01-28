#pragma once

#ifdef ENABLE_API_D3D12

#include "D3D12BackendTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Backend : public RHINOInterface {
    public:
        explicit D3D12Backend() noexcept;

    public:
        void Initialize() noexcept final;
        void Release() noexcept final;

    public:
        RTPSO* CompileRTPSO(const RTPSODesc& desc) noexcept final;
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

    public:
        void SubmitCommandList(CommandList* cmd) noexcept final;

    private:
        void SetDebugName(ID3D12DeviceChild* resource, const std::string& name) noexcept;
        ID3D12RootSignature* CreateRootSignature(size_t spacesCount, const DescriptorSpaceDesc* spaces) noexcept;

    private:
        ID3D12Device* m_Device = nullptr;
        ID3D12CommandQueue* m_DefaultQueue = nullptr;
        ID3D12Fence* m_DefaultQueueFence = nullptr;
        UINT64 m_DefaultQueueFenceLastVal = 0;
        ID3D12CommandQueue* m_ComputeQueue = nullptr;
        ID3D12Fence* m_ComputeQueueFence = nullptr;
        UINT64 m_ComputeQueueFenceLastVal = 0;
        ID3D12CommandQueue* m_CopyQueue = nullptr;
        ID3D12Fence* m_CopyQueueFence = nullptr;
        UINT64 m_CopyQueueFenceLastVal = 0;
    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
