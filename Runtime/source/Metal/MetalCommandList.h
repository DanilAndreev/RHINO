#pragma once

#ifdef ENABLE_API_METAL

#import <Metal/Metal.h>
#include "MetalBackendTypes.h"
#include "MetalDescriptorHeap.h"


namespace RHINO::APIMetal {
    constexpr size_t ROOT_SIGNATURE_RING_SIZE = 10;
    // Max size of D3D12 Root Signature in DWRODS
    // Good value to upbound size for Top Level Arg Buffer emulating Root Signature.
    constexpr size_t ROOT_SIGNATURE_SIZE_IN_RECORDS = 64;
    using RootSignatureRecordT = uint64_t;
    struct RootSignatureT {
        RootSignatureRecordT records[ROOT_SIGNATURE_SIZE_IN_RECORDS] = {};
    };

    class MetalCommandList final : public CommandList {
    private:
        id<MTLCommandBuffer> m_Cmd = nil;
        id<MTLDevice> m_Device = nil;

        MetalRootSignature* m_CurRootSignature = nullptr;
        MetalComputePSO* m_CurComputePSO = nullptr;
        MetalDescriptorHeap* m_CBVSRVUAVHeap = nullptr;
        size_t m_CBVSRVUAVHeapOffset = 0;
        MetalDescriptorHeap* m_SamplerHeap = nullptr;
        size_t m_SamplerHeapOffset = 0;

        // Top Level Argument Buffers ring emulating D3D12 Root Signatures.
        id<MTLBuffer> m_RootSignaturesRing = nil;
        id<MTLSharedEvent> m_RootSignaturesRingSync[ROOT_SIGNATURE_RING_SIZE] = {};
        size_t m_CurrentRingRootSignatureIndex = 0;
    public:
        void Initialize(id<MTLDevice> device, id<MTLCommandQueue> queue) noexcept;
        void SubmitToQueue() noexcept;

    public:
        void Dispatch(const DispatchDesc& desc) noexcept final;
        void Draw() noexcept final;
        void SetComputePSO(ComputePSO* pso) noexcept final;
        void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept final;
        void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept final;
        void DispatchRays(const DispatchRaysDesc& desc) noexcept final;
        void ResourceBarrier(const RHINO::ResourceBarrierDesc &desc) noexcept final;
        void SetRootSignature(RHINO::RootSignature *rootSignature) noexcept final;

    public:
        void Release() noexcept final;

    public:
        BLAS* BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;
        TLAS* BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset, const char* name) noexcept final;
        void BuildRTPSO(RTPSO* pso) noexcept final;
    };
} // namespace RHINO::APIMetal

#endif // ENABLE_API_METAL
