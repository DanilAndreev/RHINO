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
} // namespace RHINO::APID3D12::Convert

#endif // ENABLE_API_D3D12
