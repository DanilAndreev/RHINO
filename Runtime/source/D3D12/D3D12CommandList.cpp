#ifdef ENABLE_API_D3D12

#include "D3D12CommandList.h"
#include "D3D12BackendTypes.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Converters.h"

namespace RHINO::APID3D12 {
    void D3D12CommandList::Dispatch(const DispatchDesc& desc) noexcept {
        cmd->Dispatch(desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
    }
    void D3D12CommandList::DispatchRays() noexcept {
        D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc{};
        dispatchRaysDesc.RayGenerationShaderRecord.StartAddress =GetRayGenShader();
        dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        dispatchRaysDesc.MissShaderTable.StartAddress = GetMissShaderTable();
        dispatchRaysDesc.MissShaderTable.SizeInBytes = 1 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        dispatchRaysDesc.HitGroupTable.StartAddress = GetHitGroupTable();
        dispatchRaysDesc.HitGroupTable.SizeInBytes = 1 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        dispatchRaysDesc.Width = backbufferWidth;
        dispatchRaysDesc.Height = backbufferHeight;
        dispatchRaysDesc.Depth = 1;


        cmd->DispatchRays(&dispatchRaysDesc);
    }

    void D3D12CommandList::Draw() noexcept {}

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

    BLAS* D3D12CommandList::BuildBLAS(const BLASDesc& desc) noexcept {
        auto* indexBuffer = static_cast<D3D12Buffer*>(desc.indexBuffer);
        auto* vertexBuffer = static_cast<D3D12Buffer*>(desc.vertexBuffer);
        auto* transform = static_cast<D3D12Buffer*>(desc.transformBuffer);
        D3D12_GPU_VIRTUAL_ADDRESS trnsfrmAddr = transform ? transform->buffer->GetGPUVirtualAddress() + desc.transformBufferStartOffset : 0;

        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES};
        geometryDesc.Triangles.IndexBuffer = indexBuffer->buffer->GetGPUVirtualAddress() + desc.indexBufferStartOffset;
        geometryDesc.Triangles.IndexCount = desc.indexCount;
        geometryDesc.Triangles.IndexFormat = Convert::ToDXGIFormat(desc.indexFormat);
        geometryDesc.Triangles.Transform3x4 = trnsfrmAddr;
        geometryDesc.Triangles.VertexFormat = Convert::ToDXGIFormat(desc.vertexFormat);
        geometryDesc.Triangles.VertexCount = desc.vertexCount;
        geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->buffer->GetGPUVirtualAddress() + desc.vertexBufferStartOffset;
        geometryDesc.Triangles.VertexBuffer.StrideInBytes = desc.vertexStride;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputsDesc = {};
        inputsDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputsDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
                           D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
        inputsDesc.NumDescs = 1;
        inputsDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        inputsDesc.pGeometryDescs = &geometryDesc;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.DestAccelerationStructureData = destination;
        buildDesc.Inputs = inputsDesc;
        buildDesc.ScratchAccelerationStructureData = scratchBuffer;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildInfoDesc = {};
        postBuildInfoDesc.DestBuffer = postBuildDataBuffer;
        postBuildInfoDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

        cmd->BuildRaytracingAccelerationStructure(&buildDesc, 1, &postBuildInfoDesc);
    }

    TLAS* D3D12CommandList::BuildTLAS(const TLASDesc& desc) noexcept {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputsDesc{};
        inputsDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputsDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        inputsDesc.NumDescs = desc.instanceCount;
        inputsDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputsDesc.InstanceDescs = blasInstances;

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.DestAccelerationStructureData = destination;
        buildDesc.Inputs = inputsDesc;
        buildDesc.ScratchAccelerationStructureData = scratchBuffer;

        cmd->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    }
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
