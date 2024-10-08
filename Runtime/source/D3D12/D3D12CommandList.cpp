#ifdef ENABLE_API_D3D12

#include "D3D12CommandList.h"
#include "D3D12BackendTypes.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Converters.h"
#include "D3D12Utils.h"

namespace RHINO::APID3D12 {
    using namespace std::string_literals;

    void D3D12CommandList::Initialize(const char* name, ID3D12Device5* device, D3D12GarbageCollector* garbageCollector) noexcept {
        m_Device = device;
        m_GarbageCollector = garbageCollector;
        RHINO_D3DS(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_Allocator)));
        RHINO_D3DS(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_Allocator, nullptr, IID_PPV_ARGS(&m_Cmd)));
        RHINO_GPU_DEBUG(SetDebugName(m_Allocator, "CMDAllocator_"s + name));
        RHINO_GPU_DEBUG(SetDebugName(m_Cmd, "CMD_"s + name));

        RHINO_D3DS(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
    }

    void D3D12CommandList::Release() noexcept {
        m_Allocator->Release();
        m_Cmd->Release();
        m_Fence->Release();
        delete this;
    }

    void D3D12CommandList::SumbitToQueue(ID3D12CommandQueue* queue) noexcept {
        m_Cmd->Close();
        ID3D12CommandList* list = m_Cmd;
        queue->ExecuteCommandLists(1, &list);
        queue->Signal(m_Fence, m_FenceNextVal++);
    }

    void D3D12CommandList::Dispatch(const DispatchDesc& desc) noexcept {
        m_Cmd->Dispatch(desc.dimensionsX, desc.dimensionsY, desc.dimensionsZ);
    }
    void D3D12CommandList::DispatchRays(const DispatchRaysDesc& desc) noexcept {
        auto* d3d12PSO = static_cast<D3D12RTPSO*>(desc.pso);

        m_Cmd->SetPipelineState1(d3d12PSO->PSO);
        SetHeap(desc.CDBSRVUAVHeap, desc.samplerHeap);

        D3D12_DISPATCH_RAYS_DESC raysDesc{};
        const size_t recordStride = d3d12PSO->tableRecordStride;

        auto rayGenStart = d3d12PSO->shaderTable->GetGPUVirtualAddress() + recordStride * desc.rayGenerationShaderRecordIndex;
        raysDesc.RayGenerationShaderRecord.StartAddress = rayGenStart;
        raysDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        auto missStart = d3d12PSO->shaderTable->GetGPUVirtualAddress() + recordStride * desc.missShaderStartRecordIndex;
        raysDesc.MissShaderTable.StartAddress = missStart;
        raysDesc.MissShaderTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        auto hitGroupStart = d3d12PSO->shaderTable->GetGPUVirtualAddress() + recordStride * desc.hitGroupStartRecordIndex;
        raysDesc.HitGroupTable.StartAddress = hitGroupStart;
        raysDesc.HitGroupTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        raysDesc.Width = desc.width;
        raysDesc.Height = desc.height;
        raysDesc.Depth = 1;

        m_Cmd->DispatchRays(&raysDesc);
    }

    void D3D12CommandList::Draw() noexcept {}

    void D3D12CommandList::ResourceBarrier(const ResourceBarrierDesc& desc) noexcept {
        ID3D12Resource* resource = nullptr;
        switch (desc.resource->GetResourceType()) {
            case ResourceType::Buffer:
                resource = INTERPRET_AS<D3D12Buffer*>(desc.resource)->buffer;
            break;
            case ResourceType::Texture2D:
                resource = INTERPRET_AS<D3D12Texture2D*>(desc.resource)->texture;
            break;
            case ResourceType::Texture3D:
                resource = INTERPRET_AS<D3D12Texture3D*>(desc.resource)->texture;
            break;
            case ResourceType::BLAS:
                resource = INTERPRET_AS<D3D12BLAS*>(desc.resource)->buffer;
            break;
            case ResourceType::TLAS:
                resource = INTERPRET_AS<D3D12TLAS*>(desc.resource)->buffer;
            break;
            default:
                return;
        }

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = Convert::ToD3D12ResourceBarrierType(desc.type);
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        switch (desc.type) {
            case ResourceBarrierType::UAV:
                barrier.UAV.pResource = resource;
            break;
            case ResourceBarrierType::Transition:
                barrier.Transition.pResource = resource;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barrier.Transition.StateBefore = Convert::ToD3D12ResourceState(desc.transition.stateBefore);
            barrier.Transition.StateAfter = Convert::ToD3D12ResourceState(desc.transition.stateAfter);
            break;
        }
        m_Cmd->ResourceBarrier(1, &barrier);
    }

    void D3D12CommandList::CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept {
        auto d3d12Src = static_cast<D3D12Buffer*>(src);
        auto d3d12Dst = static_cast<D3D12Buffer*>(dst);
        m_Cmd->CopyBufferRegion(d3d12Dst->buffer, dstOffset, d3d12Src->buffer, srcOffset, size);
    }

    void D3D12CommandList::SetComputePSO(ComputePSO* pso) noexcept {
        auto* d3d12ComputePSO = static_cast<D3D12ComputePSO*>(pso);
        m_Cmd->SetPipelineState(d3d12ComputePSO->PSO);
    }

    void D3D12CommandList::SetRootSignature(RootSignature* rootSignature) noexcept {
        m_CurRootSignature = INTERPRET_AS<D3D12RootSignature*>(rootSignature);
        m_Cmd->SetComputeRootSignature(m_CurRootSignature->rootSignature);
    }

    void D3D12CommandList::SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* samplerHeap) noexcept {
        auto* d3d12CBVSRVUAVHeap = static_cast<D3D12DescriptorHeap*>(CBVSRVUAVHeap);
        auto* d3d12SamplerHeap = static_cast<D3D12DescriptorHeap*>(samplerHeap);

        if (!samplerHeap) {
            m_Cmd->SetDescriptorHeaps(1, &d3d12CBVSRVUAVHeap->GPUDescriptorHeap);
        } else {
            ID3D12DescriptorHeap* heaps[] = {d3d12CBVSRVUAVHeap->GPUDescriptorHeap, d3d12SamplerHeap->GPUDescriptorHeap};
            m_Cmd->SetDescriptorHeaps(2, heaps);
        }

        for (size_t spaceIdx = 0; spaceIdx < m_CurRootSignature->spaceDescs.size(); ++spaceIdx) {
            if (m_CurRootSignature->spaceDescs[spaceIdx].rangeDescs[0].rangeType == DescriptorRangeType::Sampler) {
                m_Cmd->SetComputeRootDescriptorTable(spaceIdx, d3d12SamplerHeap->GPUHeapGPUStartHandle);
            } else {
                m_Cmd->SetComputeRootDescriptorTable(spaceIdx, d3d12CBVSRVUAVHeap->GPUHeapGPUStartHandle);
            }
        }
    }

    void D3D12CommandList::BuildRTPSO(RTPSO* pso) noexcept {
        auto* d3d12PSO = static_cast<D3D12RTPSO*>(pso);
        assert(d3d12PSO->tableRecordStride >= D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        ID3D12StateObjectProperties* rtpsoInfo;
        d3d12PSO->PSO->QueryInterface(IID_PPV_ARGS(&rtpsoInfo));

        const size_t recordsCount = d3d12PSO->tableLayout.size();
        const size_t stagingSizeInBytes = recordsCount * d3d12PSO->tableRecordStride;
        ID3D12Resource* stagingBuffer = CreateStagingBuffer(stagingSizeInBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COPY_SOURCE);

        uint8_t* mappedData = nullptr;
        stagingBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
        for (size_t i = 0; i < recordsCount; ++i) {
            const auto& entryName = d3d12PSO->tableLayout[i].second.c_str();
            memcpy(mappedData, rtpsoInfo->GetShaderIdentifier(entryName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            mappedData += d3d12PSO->tableRecordStride;
        }
        stagingBuffer->Unmap(0, nullptr);
        m_Cmd->CopyBufferRegion(d3d12PSO->shaderTable, 0, stagingBuffer, 0, stagingSizeInBytes);
        m_GarbageCollector->AddGarbage(stagingBuffer, m_Fence, m_FenceNextVal);
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
        m_Device->GetRaytracingAccelerationStructurePrebuildInfo(&inputsDesc, &info);

        size_t ASSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        {
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resourceDesc.Width = ASSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                         D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
                                                         IID_PPV_ARGS(&result->buffer)));
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

        m_Cmd->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
        return result;
    }

    TLAS* D3D12CommandList::BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                      const char* name) noexcept {
        auto* scratch = static_cast<D3D12Buffer*>(scratchBuffer);

        auto result = new D3D12TLAS{};

        const size_t instancesSize = desc.blasInstancesCount * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        ID3D12Resource* blasInstancesCPU = CreateStagingBuffer(instancesSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COPY_SOURCE);
        RHINO_GPU_DEBUG(SetDebugName(blasInstancesCPU, "BLAS Intances CPU Staging"));
        ID3D12Resource* blasInstancesGPU = CreateStagingBuffer(instancesSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON);
        RHINO_GPU_DEBUG(SetDebugName(blasInstancesGPU, "BLAS Intances GPU Staging"));
        D3D12_RAYTRACING_INSTANCE_DESC* mappedData = nullptr;
        blasInstancesCPU->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
        for (size_t i = 0; i < desc.blasInstancesCount; ++i) {
            const BLASInstanceDesc& instanceDesc = desc.blasInstances[i];
            auto* d3d12BLAS = static_cast<D3D12BLAS*>(instanceDesc.blas);
            D3D12_RAYTRACING_INSTANCE_DESC& mappedInstanceDesc = mappedData[i];
            mappedInstanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            mappedInstanceDesc.InstanceID = instanceDesc.instanceID;
            mappedInstanceDesc.InstanceMask = instanceDesc.instanceMask;
            mappedInstanceDesc.AccelerationStructure = d3d12BLAS->buffer->GetGPUVirtualAddress();
            memcpy(mappedInstanceDesc.Transform, instanceDesc.transform, sizeof(instanceDesc.transform));
            mappedInstanceDesc.InstanceContributionToHitGroupIndex = 0; //TODO: <- figure out wtf is that
        }
        blasInstancesCPU->Unmap(0, nullptr);
        m_Cmd->CopyBufferRegion(blasInstancesGPU, 0, blasInstancesCPU, 0, instancesSize);

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = 0;
        barrier.Transition.pResource = blasInstancesGPU;
        m_Cmd->ResourceBarrier(1, &barrier);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputsDesc{};
        inputsDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputsDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputsDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        inputsDesc.NumDescs = desc.blasInstancesCount;
        inputsDesc.InstanceDescs = blasInstancesGPU->GetGPUVirtualAddress();

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
        m_Device->GetRaytracingAccelerationStructurePrebuildInfo(&inputsDesc, &info);

        size_t ASSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        {
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = ASSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                         D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
                                                         IID_PPV_ARGS(&result->buffer)));
            RHINO_GPU_DEBUG(SetDebugName(result->buffer, name));
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.DestAccelerationStructureData = result->buffer->GetGPUVirtualAddress();
        buildDesc.Inputs = inputsDesc;
        buildDesc.ScratchAccelerationStructureData = scratch->buffer->GetGPUVirtualAddress() + scratchBufferStartOffset;

        m_Cmd->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        m_GarbageCollector->AddGarbage(blasInstancesCPU, m_Fence, m_FenceNextVal);
        m_GarbageCollector->AddGarbage(blasInstancesGPU, m_Fence, m_FenceNextVal);
        return result;
    }

    ID3D12Resource* D3D12CommandList::CreateStagingBuffer(size_t size, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_STATES initialState) noexcept {
        ID3D12Resource* result = nullptr;
        // Allocating buffer for shader table
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = heap;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        if (heap == D3D12_HEAP_TYPE_DEFAULT) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Width = RHINO_CEIL_TO_MULTIPLE_OF(size, 256);
        resourceDesc.Flags = flags;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState, nullptr,
                                                     IID_PPV_ARGS(&result)));
        return result;
    }
} // namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
