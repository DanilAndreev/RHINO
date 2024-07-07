#ifdef ENABLE_API_D3D12

#include "D3D12Backend.h"
#include "D3D12CommandList.h"
#include "D3D12Converters.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Utils.h"

#include "SCARTools/SCARComputePSOArchiveView.h"

#pragma comment(lib, "dxguid.lib")
#include <dxgi.h>

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
            case TextureFormat::R32_FLOAT:
                return DXGI_FORMAT_R32_FLOAT;
            case TextureFormat::R32_UINT:
                return DXGI_FORMAT_R32_UINT;
            case TextureFormat::R32_SINT:
                return DXGI_FORMAT_R32_SINT;
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

    void D3D12Backend::Release() noexcept {}

    RTPSO* D3D12Backend::CreateRTPSO(const RTPSODesc& desc) noexcept {
        constexpr std::wstring SHADER_ID_PREFIX = L"s";
        constexpr std::wstring HITGROUP_ID_PREFIX = L"h";

        auto* result = new D3D12RTPSO{};

        std::vector<void*> allocatedObjects{};
        std::vector<std::wstring> wideNamesStorage;
        std::vector<D3D12_STATE_SUBOBJECT> subobjects{};

        // raytracing sampe (D3D12RaytracingSimpleLightning.cpp:306)
        for (size_t i = 0; i < desc.shaderModulesCount; ++i) {
            const ShaderModule& shaderModule = desc.shaderModules[i];

            std::string entrypoint = shaderModule.entrypoint;
            auto* exportDesc = new D3D12_EXPORT_DESC{};
            exportDesc->Name = wideNamesStorage.emplace_back(entrypoint.cbegin(), entrypoint.cend()).c_str();
            exportDesc->ExportToRename = wideNamesStorage.emplace_back(SHADER_ID_PREFIX + std::to_wstring(i)).c_str();
            allocatedObjects.emplace_back(exportDesc);

            auto* dxilLibDesc = new D3D12_DXIL_LIBRARY_DESC{};
            dxilLibDesc->pExports = exportDesc;
            dxilLibDesc->NumExports = 1;
            dxilLibDesc->DXILLibrary.BytecodeLength = shaderModule.bytecodeSize;
            dxilLibDesc->DXILLibrary.pShaderBytecode = shaderModule.bytecode;
            allocatedObjects.emplace_back(dxilLibDesc);
            subobjects.emplace_back(D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, dxilLibDesc});
        }

        for (size_t i = 0; i < desc.recordsCount; ++i) {
            const RTShaderTableRecord& record = desc.records[i];
            switch (record.recordType) {
                case RTShaderTableRecordType::HitGroup: {
                    D3D12_HIT_GROUP_DESC hitGroupDesc{};
                    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
                    const auto intersectionID = SHADER_ID_PREFIX + std::to_wstring(record.hitGroup.intersectionShaderIndex);
                    hitGroupDesc.IntersectionShaderImport = wideNamesStorage.emplace_back(intersectionID).c_str();
                    const auto closestHitID = SHADER_ID_PREFIX + std::to_wstring(record.hitGroup.closestHitShaderIndex);
                    hitGroupDesc.ClosestHitShaderImport = wideNamesStorage.emplace_back(closestHitID).c_str();
                    const auto anyHitID = SHADER_ID_PREFIX + std::to_wstring(record.hitGroup.anyHitShaderIndex);
                    hitGroupDesc.AnyHitShaderImport = wideNamesStorage.emplace_back(anyHitID).c_str();
                    const auto hitGroupID = HITGROUP_ID_PREFIX + std::to_wstring(i);
                    hitGroupDesc.HitGroupExport = wideNamesStorage.emplace_back(hitGroupID).c_str();
                    subobjects.emplace_back(D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc});
                    result->tableLayout.emplace_back(record.recordType, hitGroupID);
                    break;
                }
                case RTShaderTableRecordType::RayGeneration: {
                    const auto shaderID = SHADER_ID_PREFIX + std::to_wstring(record.rayGeneration.rayGenerationShaderIndex);
                    result->tableLayout.emplace_back(record.recordType, shaderID);
                    break;
                }
                case RTShaderTableRecordType::Miss: {
                    const auto shaderID = SHADER_ID_PREFIX + std::to_wstring(record.miss.missShaderIndex);
                    result->tableLayout.emplace_back(record.recordType, shaderID);
                    break;
                }
            }
        }

        //TODO: add root signature

        D3D12_RAYTRACING_SHADER_CONFIG rtShaderConfig{};
        rtShaderConfig.MaxPayloadSizeInBytes = 0; // TODO: for example pixel color or etc.
        rtShaderConfig.MaxAttributeSizeInBytes = 0; // TODO: for example barycentrics or smth

        D3D12_RAYTRACING_PIPELINE_CONFIG rtPipelineConfig{};
        rtPipelineConfig.MaxTraceRecursionDepth = desc.maxTraceRecursionDepth;

        subobjects.emplace_back(D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &rtShaderConfig});
        subobjects.emplace_back(D3D12_STATE_SUBOBJECT{D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &rtPipelineConfig});

        D3D12_STATE_OBJECT_DESC stateDesc{};
        stateDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
        stateDesc.NumSubobjects = subobjects.size();
        stateDesc.pSubobjects = subobjects.data();
        RHINO_D3DS(m_Device->CreateStateObject(&stateDesc, IID_PPV_ARGS(&result->PSO)));
        RHINO_GPU_DEBUG(SetDebugName(result->PSO, desc.debugName));

        wideNamesStorage.clear();
        for (void* ptr : allocatedObjects) {
            free(ptr);
        }
        allocatedObjects.clear();

        // Allocating buffer for shader table
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        //TODO: maybe change to D3D12_MEMORY_POOL_L0
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        // Shader table has just a pointer in the record.
        result->tableRecordSizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Width = RHINO_CEIL_TO_MULTIPLE_OF(result->tableRecordSizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
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

    ComputePSO* D3D12Backend::CompileSCARComputePSO(const void* scar, uint32_t sizeInBytes,
                                                    const char* debugName) noexcept {
        //TODO: check lang
        const SCARTools::SCARComputePSOArchiveView view{scar, sizeInBytes, debugName};
        if (!view.IsValid()) {
            return nullptr;
        }
        return CompileComputePSO(view.GetDesc());
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
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

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
        result->Initialize(name, m_Device);
        return result;
    }

    void D3D12Backend::ReleaseCommandList(CommandList* commandList) noexcept {
        if (!commandList)
            return;
        auto* d3d12CommandList = static_cast<D3D12CommandList*>(commandList);
        d3d12CommandList->m_Cmd->Release();
        d3d12CommandList->m_Allocator->Release();
        delete d3d12CommandList;
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
        WaitForSingleObject(event, INFINITE);
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
