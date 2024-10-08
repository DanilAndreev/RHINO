#ifdef ENABLE_API_D3D12

#include "D3D12Backend.h"
#include "D3D12Converters.h"
#include "D3D12Utils.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Swapchain.h"

#pragma comment(lib, "dxguid.lib")

#include <d3dx12/d3dx12.h>

namespace RHINO::APID3D12 {
    using namespace std::string_literals;

    D3D12Backend::D3D12Backend() noexcept : m_Device(nullptr) {}

    void D3D12Backend::Initialize() noexcept {
        CreateDXGIFactory(IID_PPV_ARGS(&m_DXGIFactory));

        IDXGIAdapter* adapter;
        for (UINT i = 0; m_DXGIFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC adapterDesc;
            adapter->GetDesc(&adapterDesc);
            // adapterDesc.Description;
            break;
        }

        RHINO_D3DS(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_Device)));

        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_DefaultQueue));
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_ComputeQueue));
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CopyQueue));
    }

    void D3D12Backend::Release() noexcept {
        // TODO: finish garbage collector thread and wait for it.
        m_GarbageCollector.Release();
        m_DXGIFactory->Release();
    }

    RootSignature* D3D12Backend::SerializeRootSignature(const RootSignatureDesc& desc) noexcept {
        auto* result = new D3D12RootSignature{};

        // TODO: if root constants defined: add root constants root param

        std::vector<D3D12_DESCRIPTOR_RANGE> rangeDescsStorage{};
        std::vector<D3D12_ROOT_PARAMETER> rootParamsDescs{};
        rootParamsDescs.reserve(desc.spacesCount + 1);
        std::vector<size_t> offsetsInRangeDescsPerSpaceIdx{};
        offsetsInRangeDescsPerSpaceIdx.resize(desc.spacesCount);

        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            D3D12_ROOT_PARAMETER& rootParamDesc = rootParamsDescs.emplace_back();
            rootParamDesc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParamDesc.DescriptorTable.NumDescriptorRanges = desc.spacesDescs[spaceIdx].rangeDescCount;
            offsetsInRangeDescsPerSpaceIdx[spaceIdx] = rangeDescsStorage.size();

            //TODO: assert that ranges in one space exclusively CBVSRVUAV or SMP.

            for (size_t i = 0; i < desc.spacesDescs[spaceIdx].rangeDescCount; ++i) {
                D3D12_DESCRIPTOR_RANGE& rangeDesc = rangeDescsStorage.emplace_back();
                rangeDesc.NumDescriptors = desc.spacesDescs[spaceIdx].rangeDescs[i].descriptorsCount;
                rangeDesc.RegisterSpace = desc.spacesDescs[spaceIdx].space;
                rangeDesc.OffsetInDescriptorsFromTableStart = desc.spacesDescs[spaceIdx].offsetInDescriptorsFromTableStart +
                                                              desc.spacesDescs[spaceIdx].rangeDescs[i].baseRegisterSlot;
                rangeDesc.BaseShaderRegister = desc.spacesDescs[spaceIdx].rangeDescs[i].baseRegisterSlot;
                switch (desc.spacesDescs[spaceIdx].rangeDescs[i].rangeType) {
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
            }
        }
        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            auto* rangesPtr = rangeDescsStorage.data() + offsetsInRangeDescsPerSpaceIdx[spaceIdx];
            rootParamsDescs[spaceIdx].DescriptorTable.pDescriptorRanges = rangesPtr;
        }

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = rootParamsDescs.size();
        rootSignatureDesc.pParameters = rootParamsDescs.data();
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        // rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        //                           D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        //                           D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        //D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT

        ID3DBlob* serializedRootSig = nullptr;
        RHINO_D3DS(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &serializedRootSig,
                                               nullptr));

        RHINO_D3DS(m_Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
                                                 serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&result->rootSignature)));
        serializedRootSig->Release();
        RHINO_GPU_DEBUG(SetDebugName(result->rootSignature, desc.debugName));

        // Serializing CPU root signature desc.
        result->spaceDescs.resize(desc.spacesCount);
        std::vector<size_t> offsetInRangeDescsPerDescriptorSpace{};
        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            result->spaceDescs[spaceIdx] = desc.spacesDescs[spaceIdx];
            offsetInRangeDescsPerDescriptorSpace.push_back(result->rangeDescsStorage.size());
            for (size_t i = 0; i < desc.spacesDescs[spaceIdx].rangeDescCount; ++i) {
                result->rangeDescsStorage.push_back(desc.spacesDescs[spaceIdx].rangeDescs[i]);
            }
        }
        for (size_t spaceIdx = 0; spaceIdx < desc.spacesCount; ++spaceIdx) {
            auto addr = result->rangeDescsStorage.data() + offsetInRangeDescsPerDescriptorSpace[spaceIdx];
            result->spaceDescs[spaceIdx].rangeDescs = addr;
        }

        return result;
    }

    RTPSO* D3D12Backend::CreateRTPSO(const RTPSODesc& desc) noexcept {
        static const std::wstring SHADER_ID_PREFIX = L"shdr";
        static const std::wstring HITGROUP_ID_PREFIX = L"htgrp";

        auto* d3d12RootSignature = INTERPRET_AS<D3D12RootSignature*>(desc.rootSignature);

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
        globalRootSignature->SetRootSignature(d3d12RootSignature->rootSignature);

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

    ComputePSO* D3D12Backend::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* d3d12RootSignature = INTERPRET_AS<D3D12RootSignature*>(desc.rootSignature);

        auto* result = new D3D12ComputePSO{};

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.pRootSignature = d3d12RootSignature->rootSignature;
        psoDesc.CS.pShaderBytecode = desc.CS.bytecode;
        psoDesc.CS.BytecodeLength = desc.CS.bytecodeSize;

        m_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&result->PSO));

        SetDebugName(result->PSO, desc.debugName);
        return result;
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
        heapProperties.Type = Convert::ToD3D12HeapType(heapType);
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
        resourceDesc.Flags = Convert::ToD3D12ResourceFlags(usage);

        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                                     result->currentState, nullptr, IID_PPV_ARGS(&result->buffer)));

        RHINO_GPU_DEBUG(SetDebugName(result->buffer, name));
        return result;
    }

    void* D3D12Backend::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        auto* d3d12Buffer = INTERPRET_AS<D3D12Buffer*>(buffer);
        void* result = nullptr;
        D3D12_RANGE range{};
        range.Begin = offset;
        range.End = offset + size;
        RHINO_D3DS(d3d12Buffer->buffer->Map(0, &range, &result));
        return result;
    }

    void D3D12Backend::UnmapMemory(Buffer* buffer) noexcept {
        auto* d3d12Buffer = INTERPRET_AS<D3D12Buffer*>(buffer);
        D3D12_RANGE range{};
        // range = ; //TODO: finish.
        d3d12Buffer->buffer->Unmap(0, &range);
    }

    Texture2D* D3D12Backend::CreateTexture2D(const Dim3D& dimensions, size_t mips, TextureFormat format, ResourceUsage usage,
                                             const char* name) noexcept {
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
        resourceDesc.Format = Convert::ToDXGIFormat(format);
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        resourceDesc.Width = dimensions.width;
        resourceDesc.Height = dimensions.height;

        resourceDesc.Flags = Convert::ToD3D12ResourceFlags(usage);

        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON,
                                                     nullptr, IID_PPV_ARGS(&result->texture)));

        RHINO_GPU_DEBUG(SetDebugName(result->texture, name));
        return result;
    }

    Sampler* D3D12Backend::CreateSampler(const SamplerDesc& desc) noexcept {
        auto* result = new D3D12Sampler{};
        result->samplerDesc = desc;
        return result;
    }

    DescriptorHeap* D3D12Backend::CreateDescriptorHeap(DescriptorHeapType heapType, size_t descriptorsCount, const char* name) noexcept {
        auto* result = new D3D12DescriptorHeap{};
        result->device = m_Device;

        D3D12_DESCRIPTOR_HEAP_TYPE nativeHeapType = Convert::ToD3D12DescriptorHeapType(heapType);
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

    Swapchain* D3D12Backend::CreateSwapchain(const SwapchainDesc& desc) noexcept {
        auto* result = new D3D12Swapchain{};
        result->Initialize(m_DXGIFactory, m_Device, m_DefaultQueue, desc);
        return result;
    }

    CommandList* D3D12Backend::AllocateCommandList(const char* name) noexcept {
        auto* result = new D3D12CommandList{};
        result->Initialize(name, m_Device, &m_GarbageCollector);
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
        auto d3d12CMD = INTERPRET_AS<D3D12CommandList*>(cmd);
        d3d12CMD->SumbitToQueue(m_DefaultQueue);
    }

    void D3D12Backend::SwapchainPresent(Swapchain* swapchain, Texture2D* toPresent, size_t width, size_t height) noexcept {
        auto* d3d12Swapchain = INTERPRET_AS<D3D12Swapchain*>(swapchain);
        auto* d3d12Texture = INTERPRET_AS<D3D12Texture2D*>(toPresent);
        d3d12Swapchain->Present(d3d12Texture, width, height);
    }

    Semaphore* D3D12Backend::CreateSyncSemaphore(uint64_t initialValue) noexcept {
        auto* result = new D3D12Semaphore{};
        m_Device->CreateFence(initialValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&result->fence));
        return result;
    }

    void D3D12Backend::SignalFromQueue(Semaphore* semaphore, uint64_t value) noexcept {
        auto* d3d12Semaphore = INTERPRET_AS<D3D12Semaphore*>(semaphore);
        m_DefaultQueue->Signal(d3d12Semaphore->fence, value);
    }

    void D3D12Backend::SignalFromHost(Semaphore* semaphore, uint64_t value) noexcept {
        auto* d3d12Semaphore = INTERPRET_AS<D3D12Semaphore*>(semaphore);
        d3d12Semaphore->fence->Signal(value);
    }

    bool D3D12Backend::SemaphoreWaitFromHost(const Semaphore* semaphore, uint64_t value, size_t timeout) noexcept {
        const auto* d3d12Semaphore = INTERPRET_AS<const D3D12Semaphore*>(semaphore);

        HANDLE event = CreateEventA(nullptr, true, false, "RHINO.WaitForSemaphore");
        d3d12Semaphore->fence->SetEventOnCompletion(value, event);
        const DWORD res = WaitForSingleObject(event, timeout);
        CloseHandle(event);
        return res == WAIT_OBJECT_0;
    }

    void D3D12Backend::SemaphoreWaitFromQueue(const Semaphore* semaphore, uint64_t value) noexcept {
        const auto* d3d12Semaphore = INTERPRET_AS<const D3D12Semaphore*>(semaphore);
        m_DefaultQueue->Wait(d3d12Semaphore->fence, value);
    }

    uint64_t D3D12Backend::GetSemaphoreCompletedValue(const Semaphore* semaphore) noexcept {
        const auto* d3d12Semaphore = INTERPRET_AS<const D3D12Semaphore*>(semaphore);
        return d3d12Semaphore->fence->GetCompletedValue();
    }
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
