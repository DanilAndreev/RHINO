#pragma once

#ifdef ENABLE_API_METAL

namespace RHINO::APIMetal {
    class MetalTexture2D;
    class MetalBackend : public RHINOInterface {
    private:
        id<MTLDevice> m_Device = nil;
        id<MTLCommandQueue> m_DefaultQueue;
        id<MTLCommandQueue> m_AsyncComputeQueue;
        id<MTLCommandQueue> m_CopyQueue;

    public:
        MetalBackend() noexcept {}

    public:
        void Initialize() noexcept override;
        void Release() noexcept override;

    public:
        RTPSO* CompileRTPSO(const RTPSODesc& desc) noexcept final;
        void ReleaseRTPSO(RTPSO* pso) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;
        void ReleaseComputePSO(ComputePSO* pso) noexcept final;

    public:
        MetalBuffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void ReleaseBuffer(Buffer* buffer) noexcept final;
        MetalTexture2D* CreateTexture2D() noexcept final;
        void ReleaseTexture2D(Texture2D* texture) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept final;
        void ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept final;
        CommandList* AllocateCommandList(const char* name) noexcept override;
        void ReleaseCommandList(CommandList* commandList) noexcept override;

    public:
        // JOB SUBMISSION
        virtual void SubmitCommandList(CommandList* cmd) noexcept final;
    };

    RHINOInterface* AllocateMetalBackend() noexcept {
        return new MetalBackend{};
    }
}// namespace RHINO::APIMetal

#endif// ENABLE_API_METAL