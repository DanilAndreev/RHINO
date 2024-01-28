#ifdef ENABLE_API_D3D12

#include "D3D12CommandList.h"
#include "D3D12BackendTypes.h"
#include "D3D12DescriptorHeap.h"

namespace RHINO::APID3D12 {
    void D3D12CommandList::Dispatch(const DispatchDesc& desc) noexcept {
        cmd->Dispatch(desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
    }

    void D3D12CommandList::Draw() noexcept {
    }

    void D3D12CommandList::CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept {\
        auto d3d12Src = static_cast<D3D12Buffer*>(src);
        auto d3d12Dst = static_cast<D3D12Buffer*>(dst);
        cmd->CopyBufferRegion(d3d12Dst->buffer, dstOffset, d3d12Src->buffer, srcOffset, size);
    }

    void D3D12CommandList::SetComputePSO(ComputePSO* pso) noexcept {
        auto* d3d12ComputePSO = static_cast<D3D12ComputePSO*>(pso);
        cmd->SetComputeRootSignature(d3d12ComputePSO->rootSignature);
        cmd->SetPipelineState(d3d12ComputePSO->PSO);
    }

    void D3D12CommandList::SetRTPSO(RTPSO* pso) noexcept {
    }

    void D3D12CommandList::SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept {
        auto* d3d12CBVSRVUAVHeap = static_cast<D3D12DescriptorHeap*>(CBVSRVUAVHeap);
        auto* d3d12SamplerHeap = static_cast<D3D12DescriptorHeap*>(samplerHeap);
        if (!samplerHeap) {
            cmd->SetDescriptorHeaps(1, &d3d12CBVSRVUAVHeap->GPUDescriptorHeap);
            cmd->SetComputeRootDescriptorTable(0, d3d12CBVSRVUAVHeap->GPUHeapGPUStartHandle);
        } else {
            ID3D12DescriptorHeap* heaps[] = {d3d12CBVSRVUAVHeap->GPUDescriptorHeap, d3d12SamplerHeap->GPUDescriptorHeap};
            cmd->SetDescriptorHeaps(2, heaps);
            cmd->SetComputeRootDescriptorTable(0, d3d12CBVSRVUAVHeap->GPUHeapGPUStartHandle);
            cmd->SetComputeRootDescriptorTable(1, d3d12SamplerHeap->GPUHeapGPUStartHandle);
        }
    }
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
