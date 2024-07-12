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

    class MetalComputePSO : public ComputePSO {
    public:
        id<MTLComputePipelineState> pso = nil;

        // ROOT SIGNATURE
        std::vector<DescriptorSpaceDesc> spaceDescs{};
        // Just vector with all root signature ranges stored. Read by space from  spaceDescs.
        std::vector<DescriptorRangeDesc> rangeDescsStorage{};
    };

    class MetalBLAS : public BLAS {
    public:
        id<MTLAccelerationStructure> accelerationStructure = nil;
    };

    class MetalTLAS : public TLAS {
    public:
        id<MTLAccelerationStructure> accelerationStructure = nil;
    };
}// namespace RHINO::APIMetal

#endif// ENABLE_API_METAL