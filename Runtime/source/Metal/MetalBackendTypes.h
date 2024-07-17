#pragma once

#ifdef ENABLE_API_METAL

#import <Metal/Metal.h>
#include "RHINOTypesImpl.h"

#include <metal_irconverter/metal_irconverter.h>

namespace RHINO::APIMetal {
    class MetalBuffer : public BufferBase {
    public:
        id<MTLBuffer> buffer = nil;

    public:
        void Release() noexcept final {
            delete this;
        }
    };

    class MetalTexture2D : public Texture2DBase {
    public:
        id<MTLTexture> texture;

    public:
        void Release() noexcept final {
            delete this;
        }
    };

    class MetalTexture3D : public Texture3DBase {
    public:
        id<MTLTexture> texture;

    public:
        void Release() noexcept final {
            delete this;
        }
    };

    class MetalRootSignature : public RootSignature {
    public:
        IRRootSignature* rootSignature = nullptr;

        std::vector<DescriptorSpaceDesc> spaceDescs{};
        // Just vector with all root signature ranges stored. Read by space from  spaceDescs.
        std::vector<DescriptorRangeDesc> rangeDescsStorage{};

    public:
        void Release() noexcept final {
            IRRootSignatureDestroy(this->rootSignature);
            delete this;
        }
    };

    class MetalRTPSO : public RTPSO {
    public:
    public:
        void Release() noexcept final {
            delete this;
        }
    };

    class MetalComputePSO : public ComputePSO {
    public:
        id<MTLComputePipelineState> pso = nil;
        uint32_t localWorkgroupSize[3] = {};

    public:
        void Release() noexcept final {
            delete this;
        }
    };

    class MetalBLAS : public BLASBase {
    public:
        id<MTLAccelerationStructure> accelerationStructure = nil;

    public:
        void Release() noexcept final {
            delete this;
        }
    };

    class MetalTLAS : public TLASBase {
    public:
        id<MTLAccelerationStructure> accelerationStructure = nil;

    public:
        void Release() noexcept final {
            delete this;
        }
    };

    class MetalSemaphore : public Semaphore {
    public:
        id<MTLSharedEvent> event;

    public:
        void Release() noexcept final {
            delete this;
        }
    };
}// namespace RHINO::APIMetal

#endif// ENABLE_API_METAL