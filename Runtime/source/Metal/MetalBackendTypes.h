#pragma once

#ifdef ENABLE_API_METAL

#import <Metal/Metal.h>

namespace RHINO::APIMetal {
    class MetalBuffer : public Buffer {
    public:
        id<MTLBuffer> buffer = nil;
    };

    class MetalTexture2D : public Texture2D {
    public:
        id<MTLTexture> texture;
    };

    class MetalRTPSO : public RTPSO {
    public:
    };
}// namespace RHINO::APIMetal

#endif// ENABLE_API_METAL