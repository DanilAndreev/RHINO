#pragma once
#include "RHINOTypesImpl.h"

#ifdef ENABLE_API_D3D12

#include "RHINOTypes.h"

namespace RHINO::APID3D12 {
    class D3D12Buffer : public BufferBase {
    public:
        ID3D12Resource* buffer = nullptr;
        D3D12_RESOURCE_DESC desc;
        D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_COMMON;

    public:
        void Release() noexcept final {
            this->buffer->Release();
            delete this;
        }
    };

    class D3D12Texture2D : public Texture2DBase {
    public:
        ID3D12Resource* texture = nullptr;
        D3D12_RESOURCE_DESC desc;

    public:
        void Release() noexcept final {
            this->texture->Release();
            delete this;
        }
    };

    class D3D12Texture3D : public Texture2DBase {
    public:
        ID3D12Resource* texture = nullptr;
        D3D12_RESOURCE_DESC desc;

    public:
        void Release() noexcept final {
            this->texture->Release();
            delete this;
        }
    };

    class D3D12BLAS : public BLASBase {
    public:
        ID3D12Resource* buffer = nullptr;

    public:
        void Release() noexcept final {
            this->buffer->Release();
            delete this;
        }
    };

    class D3D12TLAS : public TLASBase {
    public:
        ID3D12Resource* buffer = nullptr;

    public:
        void Release() noexcept final {
            this->buffer->Release();
            delete this;
        }
    };

    class D3D12RTPSO : public RTPSO {
    public:
        ID3D12StateObject* PSO = nullptr;
        ID3D12RootSignature* rootSignature = nullptr;
        ID3D12Resource* shaderTable = nullptr;
        std::vector<std::pair<RTShaderTableRecordType, std::wstring>> tableLayout;
        size_t tableRecordStride = 0;

    public:
        void Release() noexcept final {
            if (this->PSO)
                this->PSO->Release();
            if (this->shaderTable)
                this->shaderTable->Release();
            delete this;
        }
    };

    class D3D12ComputePSO : public ComputePSO {
    public:
        ID3D12RootSignature* rootSignature = nullptr;
        ID3D12PipelineState* PSO = nullptr;

    public:
        void Release() noexcept final {
            this->PSO->Release();
            this->rootSignature->Release();
            delete this;
        }
    };

    class D3D12Semaphore : public Semaphore {
    public:
        ID3D12Fence* fence = nullptr;

    public:
        void Release() noexcept final {
            this->fence->Release();
            delete this;
        }
    };
} // namespace RHINO::APID3D12

#endif // ENABLE_API_D3D12
