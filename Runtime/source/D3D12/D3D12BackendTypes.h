#pragma once
#include "RHINOTypesImpl.h"

#ifdef ENABLE_API_D3D12

#include "RHINOTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Buffer : public Buffer {
    public:
        ID3D12Resource* buffer = nullptr;
        D3D12_RESOURCE_DESC desc;
        D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;
    };

    class D3D12Texture2D : public Texture2D {
    public:
        ID3D12Resource* texture = nullptr;
        D3D12_RESOURCE_DESC desc;
        void* mapped = nullptr;
    };

    class D3D12BLAS : public BLAS {
    public:
        ID3D12Resource* buffer = nullptr;
    };

    class D3D12TLAS : public TLAS {
    public:
        ID3D12Resource* buffer = nullptr;
    };

    class D3D12RTPSO : public RTPSO {
    public:
        ID3D12StateObject* PSO = nullptr;
        ID3D12Resource* shaderTable = nullptr;
        std::vector<std::pair<RTShaderTableRecordType, std::wstring>> tableLayout;
        size_t tableRecordSizeInBytes = 0;
    };

    class D3D12ComputePSO : public ComputePSO {
    public:
        ID3D12RootSignature* rootSignature = nullptr;
        ID3D12PipelineState* PSO = nullptr;
    };

    class D3D12FenceInternal {
        ID3D12Fence* fence = nullptr;
        
    };

    class D3D12Fence : public Fence {
        std::shared_ptr<D3D12FenceInternal> fenceInternal;
    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
