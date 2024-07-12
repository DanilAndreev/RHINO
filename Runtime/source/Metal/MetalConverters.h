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
} // namespace RHINO::APIMetal::Convert

#endif // ENABLE_API_METAL
