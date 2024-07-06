#ifdef ENABLE_API_D3D12

#include "D3D12CommandList.h"
#include "D3D12BackendTypes.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Converters.h"
#include "D3D12Utils.h"

namespace RHINO::APID3D12 {
    void D3D12CommandList::Dispatch(const DispatchDesc& desc) noexcept {
        cmd->Dispatch(desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
    }
    void D3D12CommandList::DispatchRays() noexcept {
        D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc{};
        // dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = GetRayGenShader();
        // dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        // dispatchRaysDesc.MissShaderTable.StartAddress = GetMissShaderTable();
        // dispatchRaysDesc.MissShaderTable.SizeInBytes = 1 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        // dispatchRaysDesc.HitGroupTable.StartAddress = GetHitGroupTable();
        // dispatchRaysDesc.HitGroupTable.SizeInBytes = 1 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        // dispatchRaysDesc.Width = backbufferWidth;
        // dispatchRaysDesc.Height = backbufferHeight;
        // dispatchRaysDesc.Depth = 1;


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

    BLAS* D3D12CommandList::BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                      const char* name) noexcept {
        auto* indexBuffer = static_cast<D3D12Buffer*>(desc.indexBuffer);
        auto* vertexBuffer = static_cast<D3D12Buffer*>(desc.vertexBuffer);
        auto* transform = static_cast<D3D12Buffer*>(desc.transformBuffer);
        auto* scratch = static_cast<D3D12Buffer*>(scratchBuffer);

        auto result = new D3D12BLAS{};

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

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&inputsDesc, &info);

        size_t ASSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        {
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
            resourceDesc.Width = ASSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;

            RHINO_D3DS(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&result->buffer)));
            RHINO_GPU_DEBUG(SetDebugName(result->buffer, name));
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.DestAccelerationStructureData = result->buffer->GetGPUVirtualAddress();
        buildDesc.Inputs = inputsDesc;
        buildDesc.ScratchAccelerationStructureData = scratch->buffer->GetGPUVirtualAddress() + scratchBufferStartOffset;

        //TODO: retrieve compacted size and repack
        // D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildInfoDesc = {};
        // postBuildInfoDesc.DestBuffer = postBuildDataBuffer;
        // postBuildInfoDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
        // cmd->BuildRaytracingAccelerationStructure(&buildDesc, 1, &postBuildInfoDesc);

        cmd->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
        return result;
    }

    TLAS* D3D12CommandList::BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                      const char* name) noexcept {
        auto* scratch = static_cast<D3D12Buffer*>(scratchBuffer);

        auto result = new D3D12TLAS{};

        std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instancesDesc{desc.blasInstancesCount};

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputsDesc{};
        inputsDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputsDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputsDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        inputsDesc.NumDescs = desc.blasInstancesCount;
        inputsDesc.InstanceDescs = blasInstances;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&inputsDesc, &info);

        size_t ASSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        {
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
            resourceDesc.Width = ASSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;

            RHINO_D3DS(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&result->buffer)));
            RHINO_GPU_DEBUG(SetDebugName(result->buffer, name));
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.DestAccelerationStructureData = result->buffer->GetGPUVirtualAddress();
        buildDesc.Inputs = inputsDesc;
        buildDesc.ScratchAccelerationStructureData = scratch->buffer->GetGPUVirtualAddress() + scratchBufferStartOffset;

        cmd->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
        return result;
    }
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
