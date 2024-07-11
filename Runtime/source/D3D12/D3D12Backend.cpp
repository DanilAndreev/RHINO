#ifdef ENABLE_API_D3D12

#include "D3D12Backend.h"
#include "D3D12CommandList.h"
#include "D3D12Converters.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Utils.h"

#pragma comment(lib, "dxguid.lib")
#include <dxgi.h>

#include <d3dx12/d3dx12.h>

namespace RHINO::APID3D12 {
    using namespace std::string_literals;

    static D3D12_HEAP_TYPE ToD3D12HeapType(ResourceHeapType value) noexcept {
        switch (value) {
            case ResourceHeapType::Default:
                return D3D12_HEAP_TYPE_DEFAULT;
            case ResourceHeapType::Upload:
                return D3D12_HEAP_TYPE_UPLOAD;
            case ResourceHeapType::Readback:
                return D3D12_HEAP_TYPE_READBACK;
            default:
                assert(0);
                return D3D12_HEAP_TYPE_DEFAULT;
        }
    }

    static D3D12_RESOURCE_FLAGS ToD3D12ResourceFlags(ResourceUsage value) noexcept {
        D3D12_RESOURCE_FLAGS nativeFlags = D3D12_RESOURCE_FLAG_NONE;
        if (bool(value & ResourceUsage::UnorderedAccess))
            nativeFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        return nativeFlags;
    }

    static D3D12_DESCRIPTOR_HEAP_TYPE ToD3D12DescriptorHeapType(DescriptorHeapType type) noexcept {
        switch (type) {
            case DescriptorHeapType::SRV_CBV_UAV:
                return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            case DescriptorHeapType::RTV:
                return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            case DescriptorHeapType::DSV:
                return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            case DescriptorHeapType::Sampler:
                return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            default:
                assert(0);
                return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        }
    }

    static DXGI_FORMAT ToDXGIFormat(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R32G32B32A32_FLOAT:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case TextureFormat::R32G32B32A32_UINT:
                return DXGI_FORMAT_R32G32B32A32_UINT;
            case TextureFormat::R32G32B32A32_SINT:
                return DXGI_FORMAT_R32G32B32A32_SINT;
            case TextureFormat::R32G32B32_FLOAT:
                return DXGI_FORMAT_R32G32B32_FLOAT;
            case TextureFormat::R32G32B32_UINT:
                return DXGI_FORMAT_R32G32B32_UINT;
            case TextureFormat::R32G32B32_SINT:
                return DXGI_FORMAT_R32G32B32_SINT;
            case TextureFormat::R32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;
            case TextureFormat::R32_UINT:
                return DXGI_FORMAT_R32_UINT;
            case TextureFormat::R32_SINT:
                return DXGI_FORMAT_R32_SINT;
            default:
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }

    D3D12Backend::D3D12Backend() noexcept : m_Device(nullptr) {}

    void D3D12Backend::Initialize() noexcept {
        IDXGIFactory* factory = nullptr;
        CreateDXGIFactory(IID_PPV_ARGS(&factory));

        IDXGIAdapter* adapter;
        for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC adapterDesc;
            adapter->GetDesc(&adapterDesc);
            // adapterDesc.Description;
            break;
        }

        RHINO_D3DS(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_Device)));
        factory->Release();

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_DefaultQueue));
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_ComputeQueue));
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CopyQueue));

        m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DefaultQueueFence));
        m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_ComputeQueueFence));
        m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_CopyQueueFence));
    }

    void D3D12Backend::Release() noexcept {
        //TODO: finish garbage collector thread and wait for it.
        m_GarbageCollector.Release();
    }

    RTPSO* D3D12Backend::CreateRTPSO(const RTPSODesc& desc) noexcept {
        static const std::wstring SHADER_ID_PREFIX = L"shdr";
        static const std::wstring HITGROUP_ID_PREFIX = L"htgrp";

        auto* result = new D3D12RTPSO{};

        std::vector<std::wstring> wStrg;
        std::vector<D3D12_SHADER_BYTECODE> bytecodeRefs{};

        CD3DX12_STATE_OBJECT_DESC raytracingPipeline{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};
        for (size_t i = 0; i < desc.shaderModulesCount; ++i) {
            const ShaderModule& shaderModule = desc.shaderModules[i];
            auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            D3D12_SHADER_BYTECODE& libdxil = bytecodeRefs.emplace_back(shaderModule.bytecode, shaderModule.bytecodeSize);
            lib->SetDXILLibrary(&libdxil);
            const std::string entrypoint = shaderModule.entrypoint;
            const auto wEntrypoint = wStrg.emplace_back(entrypoint.cbegin(), entrypoint.cend()).c_str();
            const auto exportName = wStrg.emplace_back(SHADER_ID_PREFIX + std::to_wstring(i)).c_str();
            lib->DefineExport(exportName, wEntrypoint);
        }

        for (size_t i = 0; i < desc.recordsCount; ++i) {
            const RTShaderTableRecord& record = desc.records[i];
            switch (record.recordType) {
                case RTShaderTableRecordType::RayGeneration: {
                    const auto shaderID = SHADER_ID_PREFIX + std::to_wstring(record.rayGeneration.rayGenerationShaderIndex);
                    result->tableLayout.emplace_back(record.recordType, shaderID);
                    break;
                }
                case RTShaderTableRecordType::HitGroup: {
                    auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                    if (record.hitGroup.intersectionShaderIndex) {
                        const auto shaderID = wStrg.emplace_back(SHADER_ID_PREFIX + std::to_wstring(record.hitGroup.intersectionShaderIndex)).c_str();
                        hitGroup->SetIntersectionShaderImport(shaderID);
                    }
                    if (record.hitGroup.clothestHitShaderEnabled) {
                        const auto shaderID = wStrg.emplace_back(SHADER_ID_PREFIX + std::to_wstring(record.hitGroup.closestHitShaderIndex)).c_str();
                        hitGroup->SetClosestHitShaderImport(shaderID);
                    }
                    if (record.hitGroup.anyHitShaderEnabled) {
                        const auto shaderID = wStrg.emplace_back(SHADER_ID_PREFIX + std::to_wstring(record.hitGroup.anyHitShaderIndex)).c_str();
                        hitGroup->SetAnyHitShaderImport(shaderID);
                    }
                    const auto hitGroupID = wStrg.emplace_back(HITGROUP_ID_PREFIX + std::to_wstring(i)).c_str();
                    hitGroup->SetHitGroupExport(hitGroupID);
                    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

                    result->tableLayout.emplace_back(record.recordType, hitGroupID);
                    break;
                }
                case RTShaderTableRecordType::Miss: {
                    const auto shaderID = SHADER_ID_PREFIX + std::to_wstring(record.miss.missShaderIndex);
                    result->tableLayout.emplace_back(record.recordType, shaderID);
                    break;
                }
            }
        }

        auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        shaderConfig->Config(desc.maxPayloadSizeInBytes, desc.maxAttributeSizeInBytes);

        auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        result->rootSignature = CreateRootSignature(desc.spacesCount, desc.spacesDescs);
        globalRootSignature->SetRootSignature(result->rootSignature);

        auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        pipelineConfig->Config(desc.maxTraceRecursionDepth);

        const D3D12_STATE_OBJECT_DESC* pDesc = raytracingPipeline;
        RHINO_D3DS(m_Device->CreateStateObject(pDesc, IID_PPV_ARGS(&result->PSO)));
        RHINO_GPU_DEBUG(SetDebugName(result->PSO, desc.debugName));

        wStrg.clear();
        bytecodeRefs.clear();

        // Allocating buffer for shader table
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        //TODO: maybe change to D3D12_MEMORY_POOL_L0
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        // Shader table has just a pointer in the record.
        result->tableRecordStride = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Width = RHINO_CEIL_TO_MULTIPLE_OF(result->tableLayout.size() * result->tableRecordStride, 256);
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
                                                     nullptr, IID_PPV_ARGS(&result->shaderTable)));

        return result;
    }

    void D3D12Backend::ReleaseRTPSO(RTPSO* pso) noexcept {
        auto* d3d12PSO = static_cast<D3D12RTPSO*>(pso);
        if (d3d12PSO->PSO)
            d3d12PSO->PSO->Release();
        if (d3d12PSO->shaderTable)
            d3d12PSO->shaderTable->Release();
        delete d3d12PSO;
    }

    ComputePSO* D3D12Backend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* result = new D3D12ComputePSO{};
        result->rootSignature = CreateRootSignature(desc.spacesCount, desc.spacesDescs);

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = result->rootSignature;
        psoDesc.CS.pShaderBytecode = desc.CS.bytecode;
        psoDesc.CS.BytecodeLength = desc.CS.bytecodeSize;

        m_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&result->PSO));

        SetDebugName(result->PSO, desc.debugName);
        SetDebugName(result->rootSignature, desc.debugName + ".RootSignature"s);
        return result;
    }

    void D3D12Backend::ReleaseComputePSO(ComputePSO* pso) noexcept {
        auto* d3d12PSO = static_cast<D3D12ComputePSO*>(pso);
        d3d12PSO->PSO->Release();
        d3d12PSO->rootSignature->Release();
    }

    Buffer* D3D12Backend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage,
                                       size_t structuredStride, const char* name) noexcept {
        RHINO_UNUSED_VAR(structuredStride);

        auto* result = new D3D12Buffer{};

        switch (heapType) {
            case ResourceHeapType::Upload:
                result->currentState = D3D12_RESOURCE_STATE_GENERIC_READ;
                break;
            case ResourceHeapType::Readback:
                result->currentState = D3D12_RESOURCE_STATE_COPY_DEST;
                break;
            default:
                result->currentState = D3D12_RESOURCE_STATE_COMMON;
        }

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = ToD3D12HeapType(heapType);
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        resourceDesc.Width = RHINO_CEIL_TO_MULTIPLE_OF(size, 256);
        resourceDesc.Flags = ToD3D12ResourceFlags(usage);

        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                     result->currentState, nullptr, IID_PPV_ARGS(&result->buffer)));
        result->desc = result->buffer->GetDesc();

        RHINO_GPU_DEBUG(SetDebugName(result->buffer, name));
        return result;
    }

    void D3D12Backend::ReleaseBuffer(Buffer* buffer) noexcept {
        if (!buffer)
            return;
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(buffer);
        d3d12Buffer->buffer->Release();
        delete d3d12Buffer;
    }

    void* D3D12Backend::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(buffer);
        void* result = nullptr;
        D3D12_RANGE range{};
        range.Begin = offset;
        range.End = offset + size;
        RHINO_D3DS(d3d12Buffer->buffer->Map(0, &range, &result));
        return result;
    }

    void D3D12Backend::UnmapMemory(Buffer* buffer) noexcept {
        auto* d3d12Buffer = static_cast<D3D12Buffer*>(buffer);
        D3D12_RANGE range{};
        // range = ; //TODO: finish.
        d3d12Buffer->buffer->Unmap(0, &range);
    }

    Texture2D* D3D12Backend::CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format,
                                             ResourceUsage usage, const char* name) noexcept {
        auto result = new D3D12Texture2D{};

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = mips;
        resourceDesc.Format = ToDXGIFormat(format);
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        resourceDesc.Width = dimensions.width;
        resourceDesc.Height = dimensions.height;

        resourceDesc.Flags = ToD3D12ResourceFlags(usage);

        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                     D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                     IID_PPV_ARGS(&result->texture)));
        result->desc = result->texture->GetDesc();

        RHINO_GPU_DEBUG(SetDebugName(result->texture, name));
        return result;
    }

    void D3D12Backend::ReleaseTexture2D(Texture2D* texture) noexcept {
        if (!texture)
            return;
        auto* d3d12Texture = static_cast<D3D12Texture2D*>(texture);
        d3d12Texture->texture->Release();
        delete d3d12Texture;
    }

    DescriptorHeap* D3D12Backend::CreateDescriptorHeap(DescriptorHeapType heapType, size_t descriptorsCount,
                                                       const char* name) noexcept {
        auto* result = new D3D12DescriptorHeap{};
        result->device = m_Device;

        D3D12_DESCRIPTOR_HEAP_TYPE nativeHeapType = ToD3D12DescriptorHeapType(heapType);
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = descriptorsCount;
        heapDesc.Type = nativeHeapType;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        RHINO_D3DS(m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&result->CPUDescriptorHeap)));

        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        RHINO_D3DS(m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&result->GPUDescriptorHeap)));

        result->descriptorHandleIncrementSize = m_Device->GetDescriptorHandleIncrementSize(nativeHeapType);

        result->CPUHeapCPUStartHandle = result->CPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

        result->GPUHeapCPUStartHandle = result->GPUDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        result->GPUHeapGPUStartHandle = result->GPUDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

        SetDebugName(result->CPUDescriptorHeap, name + ".CPUDescriptorHeap"s);
        SetDebugName(result->GPUDescriptorHeap, name + ".GPUDescriptorHeap"s);
        return result;
    }

    void D3D12Backend::ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept {
        if (!heap)
            return;
        delete heap;
    }

    CommandList* D3D12Backend::AllocateCommandList(const char* name) noexcept {
        auto* result = new D3D12CommandList{};
        result->Initialize(name, m_Device, &m_GarbageCollector);
        return result;
    }

    void D3D12Backend::ReleaseCommandList(CommandList* commandList) noexcept {
        if (!commandList)
            return;
        auto* d3d12CommandList = static_cast<D3D12CommandList*>(commandList);
        d3d12CommandList->Release();
        delete d3d12CommandList;
    }

    Semaphore* D3D12Backend::CreateSyncSemaphore(uint64_t initialValue) noexcept {
        auto* result = new D3D12Semaphore{};
        m_Device->CreateFence(initialValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&result->fence));
        return result;
    }

    ASPrebuildInfo D3D12Backend::GetBLASPrebuildInfo(const BLASDesc& desc) noexcept {
        // GetRaytracingAccelerationStructurePrebuildInfo may check pointers for null values for calculating sizes but not accessing data by
        // these pointers. So we can pass dummy not null pointers to tell D3D12 that we are about to pass real pointers during the build.
        // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-getraytracingaccelerationstructureprebuildinfo
        constexpr D3D12_GPU_VIRTUAL_ADDRESS dummyNotNullPointer = 0x1;

        D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
        geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        geometryDesc.Triangles.IndexBuffer = dummyNotNullPointer;
        geometryDesc.Triangles.IndexCount = desc.indexCount;
        geometryDesc.Triangles.IndexFormat = Convert::ToDXGIFormat(desc.indexFormat);
        geometryDesc.Triangles.Transform3x4 = 0;
        geometryDesc.Triangles.VertexFormat = Convert::ToDXGIFormat(desc.vertexFormat);
        geometryDesc.Triangles.VertexCount = desc.vertexCount;
        geometryDesc.Triangles.VertexBuffer.StartAddress = dummyNotNullPointer;
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

        auto scratchSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        auto BLASSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        return {scratchSize, BLASSize};
    }

    ASPrebuildInfo D3D12Backend::GetTLASPrebuildInfo(const TLASDesc& desc) noexcept {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputsDesc = {};
        inputsDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        inputsDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        inputsDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        inputsDesc.NumDescs = desc.blasInstancesCount;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
        m_Device->GetRaytracingAccelerationStructurePrebuildInfo(&inputsDesc, &info);

        auto scratchSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        auto BLASSize = RHINO_CEIL_TO_POWER_OF_TWO(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        return {scratchSize, BLASSize};
    }

    void D3D12Backend::SubmitCommandList(CommandList* cmd) noexcept {
        auto d3d12CMD = static_cast<D3D12CommandList*>(cmd);
        d3d12CMD->SumbitToQueue(m_DefaultQueue);
        m_DefaultQueue->Signal(m_DefaultQueueFence, ++m_CopyQueueFenceLastVal);

        HANDLE event = CreateEventA(nullptr, true, false, "DefaultQueueCompletion");
        m_DefaultQueueFence->SetEventOnCompletion(m_CopyQueueFenceLastVal, event);
        DWORD res = WaitForSingleObject(event, INFINITE);
        assert(res == WAIT_OBJECT_0);
        CloseHandle(event);
    }

    void D3D12Backend::QueueSignal(Semaphore* semaphore, uint64_t value) noexcept {
        auto* d3d12Semaphore = static_cast<D3D12Semaphore*>(semaphore);
        m_DefaultQueue->Signal(d3d12Semaphore->fence, value);
    }

    bool D3D12Backend::WaitForSemaphore(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept {
        const auto* d3d12Semaphore = static_cast<const D3D12Semaphore*>(semaphore);

        HANDLE event = CreateEventA(nullptr, true, false, "RHINO.WaitForSemaphore");
        d3d12Semaphore->fence->SetEventOnCompletion(value, event);
        DWORD res = WaitForSingleObject(event, timeout);
        CloseHandle(event);
        return res == WAIT_OBJECT_0;
    }

    ID3D12RootSignature* D3D12Backend::CreateRootSignature(size_t spacesCount,
                                                           const DescriptorSpaceDesc* spaces) noexcept {
        std::vector<D3D12_DESCRIPTOR_RANGE> rangeDescs{};

        for (size_t spaceIdx = 0; spaceIdx < spacesCount; ++spaceIdx) {
            for (size_t i = 0; i < spaces[spaceIdx].rangeDescCount; ++i) {
                D3D12_DESCRIPTOR_RANGE rangeDesc{};
                rangeDesc.NumDescriptors = spaces[spaceIdx].rangeDescs[i].descriptorsCount;
                rangeDesc.RegisterSpace = spaces[spaceIdx].space;
                rangeDesc.OffsetInDescriptorsFromTableStart = spaces[spaceIdx].offsetInDescriptorsFromTableStart +
                                                              spaces[spaceIdx].rangeDescs[i].baseRegisterSlot;
                rangeDesc.BaseShaderRegister = spaces[spaceIdx].rangeDescs[i].baseRegisterSlot;
                switch (spaces[spaceIdx].rangeDescs[i].rangeType) {
                    case DescriptorRangeType::SRV:
                        rangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        break;
                    case DescriptorRangeType::UAV:
                        rangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                        break;
                    case DescriptorRangeType::CBV:
                        rangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                        break;
                    case DescriptorRangeType::Sampler:
                        rangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                        break;
                }
                rangeDescs.push_back(rangeDesc);
            }
        }

        // TODO: bind samplers as saparate table.
        D3D12_ROOT_PARAMETER rootParamDesc{};

        // Descriptor table
        rootParamDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParamDesc.DescriptorTable.NumDescriptorRanges = rangeDescs.size();
        rootParamDesc.DescriptorTable.pDescriptorRanges = rangeDescs.data();

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = 1;
        rootSignatureDesc.pParameters = &rootParamDesc;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                  D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                  D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        //D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT

        ID3DBlob* serializedRootSig = nullptr;
        RHINO_D3DS(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &serializedRootSig,
                                               nullptr));

        ID3D12RootSignature* result = nullptr;
        RHINO_D3DS(m_Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
                                                 serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&result)));
        return result;
    }
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
