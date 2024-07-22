#pragma once

#ifdef ENABLE_API_METAL

#import <Metal/Metal.h>

namespace RHINO::APIMetal::Convert {
    inline MTLIndexType ToMTLIndexType(IndexFormat format) noexcept {
        switch (format) {
            case IndexFormat::R32_UINT:
                return MTLIndexTypeUInt32;
            case IndexFormat::R16_UINT:
                return MTLIndexTypeUInt16;
            default:
                assert(0);
                return MTLIndexTypeUInt32;
        }
    }

    inline MTLPixelFormat ToMTLPixelFormat(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R8G8B8A8_UNORM:
                return MTLPixelFormatRGBA8Unorm;
            case TextureFormat::R32G32B32A32_FLOAT:
                return MTLPixelFormatRGBA32Float;
            case TextureFormat::R32G32B32A32_UINT:
                return MTLPixelFormatRGBA32Uint;
            case TextureFormat::R32G32B32A32_SINT:
                return MTLPixelFormatRGBA32Sint;
            case TextureFormat::R32G32B32_FLOAT:
                //TODO: restructure formats to match all APIs
                assert(0);
                return MTLPixelFormatRGBA32Float;
            case TextureFormat::R32G32B32_UINT:
                //TODO: restructure formats to match all APIs
                assert(0);
                return MTLPixelFormatRGBA32Uint;
            case TextureFormat::R32G32B32_SINT:
                //TODO: restructure formats to match all APIs
                assert(0);
                return MTLPixelFormatRGBA32Sint;
            case TextureFormat::R32_FLOAT:
                return MTLPixelFormatR32Float;
            case TextureFormat::R32_UINT:
                return MTLPixelFormatR32Uint;
            case TextureFormat::R32_SINT:
                return MTLPixelFormatR32Sint;
            default:
                assert(0);
                return MTLPixelFormatRGBA8Unorm;
        }
    }

    inline MTLAttributeFormat ToMTLMTLAttributeFormat(TextureFormat format) noexcept {
        switch (format) {
            case TextureFormat::R32G32B32A32_FLOAT:
                return MTLAttributeFormatFloat4;
            case TextureFormat::R32G32B32A32_UINT:
                return MTLAttributeFormatUInt4;
            case TextureFormat::R32G32B32A32_SINT:
                return MTLAttributeFormatInt4;
            case TextureFormat::R32G32B32_FLOAT:
                return MTLAttributeFormatFloat3;
            case TextureFormat::R32G32B32_UINT:
                return MTLAttributeFormatUInt3;
            case TextureFormat::R32G32B32_SINT:
                return MTLAttributeFormatInt3;
            case TextureFormat::R32_FLOAT:
                return MTLAttributeFormatFloat;
            case TextureFormat::R32_UINT:
                return MTLAttributeFormatUInt;
            case TextureFormat::R32_SINT:
                return MTLAttributeFormatInt;
            default:
                assert(0);
                return MTLAttributeFormatFloat4;
        }
    }

    inline MTLResourceUsage ToMTLResourceUsage(ResourceUsage usage) noexcept {
        MTLResourceUsage result = 0;
        if (bool(usage & ResourceUsage::VertexBuffer)) {
            result |= MTLResourceUsageRead;
        }
        if (bool(usage & ResourceUsage::IndexBuffer)) {
            result |= MTLResourceUsageRead;
        }
        if (bool(usage & ResourceUsage::ConstantBuffer)) {
            result |= MTLResourceUsageRead;
        }
        if (bool(usage & ResourceUsage::ShaderResource)) {
            result |= MTLResourceUsageRead | MTLResourceUsageSample;
        }
        if (bool(usage & ResourceUsage::UnorderedAccess)) {
            result |= MTLResourceUsageWrite;
        }
        if (bool(usage & ResourceUsage::Indirect)) {
            result |= MTLResourceUsageRead;
        }
        if (bool(usage & ResourceUsage::CopyDest)) {
            result |= MTLResourceUsageWrite;
        }
        if (bool(usage & ResourceUsage::CopySource)) {
            result |= MTLResourceUsageRead;
        }
        return result;
    }

    inline MTLSamplerBorderColor ToMTLSamplerBorderColor(BorderColor color) noexcept {
        switch (color) {
            case BorderColor::TransparentBlack:
                return MTLSamplerBorderColorTransparentBlack;
            case BorderColor::OpaqueBlack:
                return MTLSamplerBorderColorOpaqueBlack;
            case BorderColor::OpaqueWhite:
                return MTLSamplerBorderColorOpaqueWhite;
            default:
                assert(0);
                return MTLSamplerBorderColorTransparentBlack;
        }
    }

    inline MTLSamplerAddressMode ToMTLSamplerAddressMode(TextureAddressMode mode)  noexcept {
        switch (mode) {
            case TextureAddressMode::Wrap:
                return MTLSamplerAddressModeRepeat;
            case TextureAddressMode::Mirror:
                return MTLSamplerAddressModeMirrorRepeat;
            case TextureAddressMode::Clamp:
                return MTLSamplerAddressModeClampToEdge;
            case TextureAddressMode::Border:
                return MTLSamplerAddressModeClampToBorderColor;
            case TextureAddressMode::MirrorOnce:
                return MTLSamplerAddressModeMirrorClampToEdge;
            default:
                assert(0);
                return MTLSamplerAddressModeRepeat;
        }
    }

    inline MTLCompareFunction ToMTLCompareFunction(ComparisonFunction func) noexcept {
        switch (func) {
            case ComparisonFunction::Never:
                return MTLCompareFunctionNever;
            case ComparisonFunction::Less:
                return MTLCompareFunctionLess;
            case ComparisonFunction::Equal:
                return MTLCompareFunctionEqual;
            case ComparisonFunction::LessEqual:
                return MTLCompareFunctionLessEqual;
            case ComparisonFunction::Greater:
                return MTLCompareFunctionGreater;
            case ComparisonFunction::NotEqual:
                return MTLCompareFunctionNotEqual;
            case ComparisonFunction::GreaterEqual:
                return MTLCompareFunctionGreaterEqual;
            case ComparisonFunction::Always:
                return MTLCompareFunctionAlways;
            default:
                assert(0);
                return MTLCompareFunctionNever;
        }
    }

    struct MetalMinMagMipFilters {
        MTLSamplerMinMagFilter min;
        MTLSamplerMinMagFilter mag;
        MTLSamplerMipFilter mip;
    };

    inline MetalMinMagMipFilters ToMTLMinMagMipFilter(TextureFilter filter) noexcept {
        switch (filter) {
            case TextureFilter::MinMagMipPoint:
            case TextureFilter::ComparisonMinMagMipPoint:
                return {MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterNearest, MTLSamplerMipFilterNearest};
            case TextureFilter::MinMagPointMipLinear:
            case TextureFilter::ComparisonMinMagPointMipLinear:
                return {MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterNearest, MTLSamplerMipFilterLinear};
            case TextureFilter::MinPointMagLinearMipPoint:
            case TextureFilter::ComparisonMinPointMagLinearMipPoint:
                return {MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterNearest};
            case TextureFilter::MinPointMagMipLinear:
            case TextureFilter::ComparisonMinPointMagMipLinear:
                return {MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterLinear};
            case TextureFilter::MinLinearMagMipPoint:
            case TextureFilter::ComparisonMinLinearMagMipPoint:
                return {MTLSamplerMinMagFilterLinear, MTLSamplerMinMagFilterNearest, MTLSamplerMipFilterNearest};
            case TextureFilter::MinLinearMagPointMipLinear:
            case TextureFilter::ComparisonMinLinearMagPointMipLinear:
                return {MTLSamplerMinMagFilterLinear, MTLSamplerMinMagFilterNearest, MTLSamplerMipFilterLinear};
            case TextureFilter::MinMagLinearMipPoint:
            case TextureFilter::ComparisonMinMagLinearMipPoint:
                return {MTLSamplerMinMagFilterLinear, MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterNearest};
            case TextureFilter::MinMagMipLinear:
            case TextureFilter::ComparisonMinMagMipLinear:
                return {MTLSamplerMinMagFilterLinear, MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterLinear};

            case TextureFilter::Anisotrophic:
            case TextureFilter::ComparisonAnisotrophic:
                return {MTLSamplerMinMagFilterLinear, MTLSamplerMinMagFilterLinear, MTLSamplerMipFilterLinear};
            default:
                assert(0);
                return {MTLSamplerMinMagFilterNearest, MTLSamplerMinMagFilterNearest, MTLSamplerMipFilterNearest};
        }
    }

    inline bool IsFilterAnisotrophic(TextureFilter filter) noexcept {
        switch (filter) {
            case TextureFilter::Anisotrophic:
            case TextureFilter::ComparisonAnisotrophic:
                return true;
            default:
                return false;
        }
    }
} // namespace RHINO::APIMetal::Convert

#endif // ENABLE_API_METAL
