#pragma once

#ifdef ENABLE_API_D3D12

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

    class D3D12RTPSO : public RTPSO {
    public:
    };

    class D3D12ComputePSO : public ComputePSO {
    public:
        ID3D12RootSignature* rootSignature = nullptr;
        ID3D12PipelineState* PSO = nullptr;
    };
}// namespace RHINO::APID3D12

#endif// ENABLE_API_D3D12
