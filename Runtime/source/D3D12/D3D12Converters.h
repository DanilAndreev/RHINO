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

    inline D3D12_TEXTURE_ADDRESS_MODE ToD3D12TextureAddressMode(TextureAddressMode mode) noexcept {
        switch (mode) {
            case TextureAddressMode::Wrap:
                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case TextureAddressMode::Mirror:
                return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            case TextureAddressMode::Clamp:
                return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case TextureAddressMode::Border:
                return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            case TextureAddressMode::MirrorOnce:
                return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
            default:
                assert(0);
                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    }

    inline D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(ComparisonFunction func) noexcept {
        switch (func) {
            case ComparisonFunction::Never:
                return D3D12_COMPARISON_FUNC_NEVER;
            case ComparisonFunction::Less:
                return D3D12_COMPARISON_FUNC_LESS;
            case ComparisonFunction::Equal:
                return D3D12_COMPARISON_FUNC_EQUAL;
            case ComparisonFunction::LessEqual:
                return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case ComparisonFunction::Greater:
                return D3D12_COMPARISON_FUNC_GREATER;
            case ComparisonFunction::NotEqual:
                return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            case ComparisonFunction::GreaterEqual:
                return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case ComparisonFunction::Always:
                return D3D12_COMPARISON_FUNC_ALWAYS;
            default:
                assert(0);
                return D3D12_COMPARISON_FUNC_NEVER;
        }
    }

    inline D3D12_FILTER ToD3D12Filter(TextureFilter filter) noexcept {
        switch (filter) {

        }
    }

    inline void ToD3D12BorderColor(BorderColor color, float outVar[4]) {
        switch (color) {
            case BorderColor::TransparentBlack:
                outVar[0] = 0.0f;
                outVar[1] = 0.0f;
                outVar[2] = 0.0f;
                outVar[3] = 0.0f;
                return;
            case BorderColor::OpaqueBlack:
                outVar[0] = 0.0f;
                outVar[1] = 0.0f;
                outVar[2] = 0.0f;
                outVar[3] = 1.0f;
                return;
            case BorderColor::OpaqueWhite:
                outVar[0] = 1.0f;
                outVar[1] = 1.0f;
                outVar[2] = 1.0f;
                outVar[3] = 1.0f;
                return;
            default:
                assert(0);
                outVar[0] = 0.0f;
                outVar[1] = 0.0f;
                outVar[2] = 0.0f;
                outVar[3] = 0.0f;
                return;
        }
    }

} // namespace RHINO::APID3D12::Convert

#endif // ENABLE_API_D3D12