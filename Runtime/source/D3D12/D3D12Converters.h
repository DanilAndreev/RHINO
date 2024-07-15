#pragma once

#ifdef ENABLE_API_D3D12

#include "RHINOTypes.h"

namespace RHINO::APID3D12::Convert {
    inline DXGI_FORMAT ToDXGIFormat(IndexFormat format) noexcept {
        switch (format) {
            case IndexFormat::R32_UINT:
                return DXGI_FORMAT_R32_UINT;
            case IndexFormat::R16_UINT:
                return DXGI_FORMAT_R16_UINT;
            default:
                assert(0);
                return DXGI_FORMAT_R32_UINT;
        }
    }

    inline DXGI_FORMAT ToDXGIFormat(TextureFormat format) noexcept {
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
                return DXGI_FORMAT_R16_SINT;
            default:
                assert(0);
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }

    inline D3D12_RESOURCE_BARRIER_TYPE ToD3D12ResourceBarrierType(ResourceBarrierType type) noexcept {
        switch (type) {
            case ResourceBarrierType::UAV:
                return D3D12_RESOURCE_BARRIER_TYPE_UAV;
            case ResourceBarrierType::Transition:
                return D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            default:
                assert(0);
                return D3D12_RESOURCE_BARRIER_TYPE_UAV;
        }
    }

    inline D3D12_RESOURCE_STATES ToD3D12ResourceState(ResourceState state) noexcept {
        switch (state) {
            case ResourceState::ConstantBuffer:
                return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            case ResourceState::UnorderedAccess:
                return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            case ResourceState::ShaderResource:
                return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
            case ResourceState::IndirectArgument:
                return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            case ResourceState::CopyDest:
                return D3D12_RESOURCE_STATE_COPY_DEST;
            case ResourceState::CopySource:
                return D3D12_RESOURCE_STATE_COPY_SOURCE;
            case ResourceState::HostWrite:
                return D3D12_RESOURCE_STATE_COMMON;
            case ResourceState::HostRead:
                return D3D12_RESOURCE_STATE_COMMON;
            case ResourceState::RTAccelerationStructure:
                return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
            default:
                assert(0);
                return D3D12_RESOURCE_STATE_COMMON;
        }
    }

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

    inline D3D12_RESOURCE_FLAGS ToD3D12ResourceFlags(ResourceUsage value) noexcept {
        D3D12_RESOURCE_FLAGS nativeFlags = D3D12_RESOURCE_FLAG_NONE;
        if (bool(value & ResourceUsage::UnorderedAccess))
            nativeFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        return nativeFlags;
    }

    inline D3D12_DESCRIPTOR_HEAP_TYPE ToD3D12DescriptorHeapType(DescriptorHeapType type) noexcept {
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
} // namespace RHINO::APID3D12::Convert

#endif // ENABLE_API_D3D12