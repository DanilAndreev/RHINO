#ifdef ENABLE_API_D3D12

#include "D3D12Backend.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorHeap.h"

#pragma comment(lib, "dxguid.lib")
#include <dxgi.h>


#ifdef _DEBUG
#define RHINO_D3DS(expr) assert(expr == S_OK)
#define RHINO_GPU_DEBUG(expr) expr
#else
#define RHINO_D3DS(expr) expr
#define RHINO_GPU_DEBUG(expr)
#endif


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

    D3D12Backend::D3D12Backend() noexcept : m_Device(nullptr) {
    }

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
    }

    RTPSO* D3D12Backend::CompileRTPSO(const RTPSODesc& desc) noexcept {
        return nullptr;
    }

    void D3D12Backend::ReleaseRTPSO(RTPSO* pso) noexcept {
        delete pso;
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

    Buffer* D3D12Backend::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept {
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

        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, result->currentState, nullptr, IID_PPV_ARGS(&result->buffer)));
        result->desc = result->buffer->GetDesc();

        RHINO_GPU_DEBUG(SetDebugName(result->buffer, name));
        return result;
    }

    void D3D12Backend::ReleaseBuffer(Buffer* buffer) noexcept {
        if (!buffer) return;
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

        RHINO_D3DS(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&result->texture)));
        result->desc = result->texture->GetDesc();

        RHINO_GPU_DEBUG(SetDebugName(result->texture, name));
        return result;
    }

    void D3D12Backend::ReleaseTexture2D(Texture2D* texture) noexcept {
        if (!texture) return;
        auto* d3d12Texture = static_cast<D3D12Texture2D*>(texture);
        d3d12Texture->texture->Release();
        delete d3d12Texture;
    }

    DescriptorHeap* D3D12Backend::CreateDescriptorHeap(DescriptorHeapType heapType, size_t descriptorsCount, const char* name) noexcept {
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
        if (!heap) return;
        delete heap;
    }

    CommandList* D3D12Backend::AllocateCommandList(const char* name) noexcept {
        auto* result = new D3D12CommandList{};

        RHINO_D3DS(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result->allocator)));
        RHINO_D3DS(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, result->allocator, nullptr,
                                               IID_PPV_ARGS(&result->cmd)));
        // result->cmd->Close();

        SetDebugName(result->allocator, "CMDAllocator_"s + name);
        SetDebugName(result->cmd, "CMD_"s + name);
        return result;
    }

    void D3D12Backend::ReleaseCommandList(CommandList* commandList) noexcept {
        if (!commandList) return;
        auto* d3d12CommandList = static_cast<D3D12CommandList*>(commandList);
        d3d12CommandList->cmd->Release();
        d3d12CommandList->allocator->Release();
        delete d3d12CommandList;
    }
    void D3D12Backend::SubmitCommandList(CommandList* cmd) noexcept {
        auto d3d12CMD = static_cast<D3D12CommandList*>(cmd);
        d3d12CMD->cmd->Close();
        ID3D12CommandList* list = d3d12CMD->cmd;
        m_DefaultQueue->ExecuteCommandLists(1, &list);
        m_DefaultQueue->Signal(m_DefaultQueueFence, ++m_CopyQueueFenceLastVal);

        HANDLE event = CreateEventA(nullptr, true, false, "DefaultQueueCompletion");
        m_DefaultQueueFence->SetEventOnCompletion(m_CopyQueueFenceLastVal, event);
        WaitForSingleObject(event, INFINITE);
    }

    void D3D12Backend::SetDebugName(ID3D12DeviceChild* resource, const std::string& name) noexcept {
        resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.length(), name.c_str());
    }

    ID3D12RootSignature* D3D12Backend::CreateRootSignature(size_t spacesCount, const DescriptorSpaceDesc* spaces) noexcept {
        std::vector<D3D12_DESCRIPTOR_RANGE> rangeDescs{};

        for (size_t spaceIdx = 0; spaceIdx < spacesCount; ++spaceIdx) {
            for (size_t i = 0; i < spaces[spaceIdx].rangeDescCount; ++i) {
                D3D12_DESCRIPTOR_RANGE rangeDesc{};
                rangeDesc.NumDescriptors = spaces[spaceIdx].rangeDescs[i].descriptorsCount;
                rangeDesc.RegisterSpace = spaces[spaceIdx].space;
                rangeDesc.OffsetInDescriptorsFromTableStart = spaces[spaceIdx].offsetInDescriptorsFromTableStart + spaces[spaceIdx].rangeDescs[i].baseRegisterSlot;
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

        //TODO: bind samplers as saparate table.
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
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                                  D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* serializedRootSig = nullptr;
        RHINO_D3DS(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &serializedRootSig, nullptr));

        ID3D12RootSignature* result = nullptr;
        RHINO_D3DS(m_Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&result)));
        return result;
    }
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12