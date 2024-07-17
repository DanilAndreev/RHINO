#pragma once

#include <cstddef>
#include <cstdint>

#define RHINO_DECLARE_BITMASK_ENUM(enumType)                                                                           \
    enumType operator&(enumType a, enumType b);                                                                        \
    enumType operator|(enumType a, enumType b);                                                                        \
    bool operator||(bool a, enumType b);                                                                               \
    bool operator&&(bool a, enumType b);                                                                               \
    bool operator!(enumType a);

namespace RHINO {
    // ---------------------------------------------------------------------------------- ENUMS DECLARATION
    enum class ResourceType {
        Buffer,
        Texture2D,
        Texture3D,
        BLAS,
        TLAS,
        Count,
    };

    enum class TextureFormat {
        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,

        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,

        R32_FLOAT,
        R32_UINT,
        R32_SINT,
    };

    enum class IndexFormat {
        R32_UINT,
        R16_UINT,
    };

    enum class ResourceHeapType {
        Default,
        Upload,
        Readback,
    };

    enum class ResourceUsage : size_t {
        None = 0x0,
        VertexBuffer = 0x1,
        IndexBuffer = 0x2,
        ConstantBuffer = 0x4,
        ShaderResource = 0x8,
        UnorderedAccess = 0x10,
        Indirect = 0x20,
        CopySource = 0x40,
        CopyDest = 0x80,
        ValidMask = 0xFF,
    };

    enum class DescriptorHeapType {
        SRV_CBV_UAV,
        RTV,
        DSV,
        Sampler,
        Count,
    };

    enum class DescriptorType {
        BufferSRV,
        BufferUAV,
        BufferCBV,
        Texture2DSRV,
        Texture2DUAV,
        Texture3DSRV,
        Texture3DUAV,
        Sampler,
        Count,
    };

    enum class DescriptorRangeType { CBV, SRV, UAV, Sampler };

    enum class RTShaderTableRecordType {
        RayGeneration = 0,
        HitGroup = 1,
        Miss = 2,
    };

    enum class ResourceBarrierType {
        UAV,
        Transition,
    };

    enum class ResourceState {
        ConstantBuffer,
        UnorderedAccess,
        ShaderResource,
        IndirectArgument,
        CopyDest,
        CopySource,
        HostWrite,
        HostRead,
        RTAccelerationStructure,
    };

    // ---------------------------------------------------------------------------------- OBJECTS PRE-DECLARATION

    class Object {
    protected:
        virtual ~Object() = default;
    public:
        virtual void Release() noexcept = 0;
    };

    class Resource : public Object {
    public:
        virtual ResourceType GetResourceType() = 0;
    };

    class Buffer : public Resource {};
    class Texture2D : public Resource {};
    class Texture3D : public Resource {};

    class RootSignature: public Object {};
    class RTPSO : public Object {};
    class ComputePSO : public Object {};

    class BLAS : public Resource {};
    class TLAS : public Resource {};

    class Semaphore : public Object {};

    class DescriptorHeap;
    class CommandList;

    // ---------------------------------------------------------------------------------- DESC STRUCTS + INTERFACES

    struct Dim3D {
        size_t width = 0;
        size_t height = 0;
        size_t depth = 0;
    };

    struct DescriptorRangeDesc {
        DescriptorRangeType rangeType = DescriptorRangeType::CBV;
        size_t baseRegisterSlot = 0;
        size_t descriptorsCount = 0;
    };

    struct DescriptorSpaceDesc {
        size_t space = 0;
        size_t offsetInDescriptorsFromTableStart = 0;
        size_t rangeDescCount = 0;
        const DescriptorRangeDesc* rangeDescs = nullptr;
    };

    struct ShaderModule {
        size_t bytecodeSize = 0;
        const uint8_t* bytecode = nullptr;
        const char* entrypoint = nullptr;
    };

    struct ComputePSODesc {
        RootSignature* rootSignature = nullptr;
        ShaderModule CS = {};
        const char* debugName = "UnnamedComputePSO";
    };

    struct RTHitGroupDesc {
        size_t closestHitShaderIndex = 0;
        bool clothestHitShaderEnabled = false;
        size_t anyHitShaderIndex = 0;
        bool anyHitShaderEnabled = false;
        size_t intersectionShaderIndex = 0;
        bool intersectionShaderEnabled = false;
    };
    struct RTRayGenerationDesc {
        size_t rayGenerationShaderIndex = 0;
    };
    struct RTMissDesc {
        size_t missShaderIndex = 0;
    };
    struct RTShaderTableRecord {
        RTShaderTableRecordType recordType = RTShaderTableRecordType::RayGeneration;
        union {
            RTHitGroupDesc hitGroup;
            RTRayGenerationDesc rayGeneration;
            RTMissDesc miss;
        };
    };

    struct RootSignatureDesc {
        size_t spacesCount = 0;
        const DescriptorSpaceDesc* spacesDescs = nullptr;
        const char* debugName;
    };

    struct RTPSODesc {
        size_t spacesCount = 0;
        const DescriptorSpaceDesc* spacesDescs = nullptr;
        size_t shaderModulesCount = 0;
        const ShaderModule* shaderModules = nullptr;
        size_t recordsCount = 0;
        const RTShaderTableRecord* records = nullptr;
        uint32_t maxTraceRecursionDepth = 1;
        uint32_t maxPayloadSizeInBytes = 0;
        uint32_t maxAttributeSizeInBytes = 0;

        const char* debugName = "UnnamedRTPSO";
    };

    struct DispatchDesc {
        size_t dimensionsX = 1;
        size_t dimensionsY = 1;
        size_t dimensionsZ = 1;
    };

    struct DispatchRaysDesc {
        RTPSO* pso = nullptr;
        size_t width = 0;
        size_t height = 0;
        size_t rayGenerationShaderRecordIndex = 0;
        size_t missShaderStartRecordIndex = 0;
        size_t hitGroupStartRecordIndex = 0;
        DescriptorHeap* CDBSRVUAVHeap = nullptr;
        DescriptorHeap* samplerHeap = nullptr;
    };

    struct ASPrebuildInfo {
        size_t scratchBufferSizeInBytes = 0;
        size_t MaxASSizeInBytes = 0;
    };

    struct BLASDesc {
        Buffer* indexBuffer = nullptr;
        size_t indexBufferStartOffset = 0;
        size_t indexCount = 0;
        IndexFormat indexFormat = IndexFormat::R32_UINT;
        Buffer* vertexBuffer = nullptr;
        size_t vertexBufferStartOffset = 0;
        TextureFormat vertexFormat = TextureFormat::R32G32B32_FLOAT;
        size_t vertexCount = 0;
        size_t vertexStride = 0;
        // Just one tranform value.
        Buffer* transformBuffer = nullptr;
        size_t transformBufferStartOffset = 0;
    };

    struct BLASInstanceDesc {
        BLAS* blas = nullptr;
        uint32_t instanceID = 0;
        uint32_t instanceMask = ~0u;
        float transform[3][4] = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f};
    };

    struct TLASDesc {
        size_t blasInstancesCount = 0;
        const BLASInstanceDesc* blasInstances = nullptr;
    };

    struct ResourceTransitionBarrierDesc {
        ResourceState stateBefore;
        ResourceState stateAfter;
    };
    struct ResourceBarrierDesc {
        ResourceBarrierType type;
        Resource* resource;
        union {
            ResourceTransitionBarrierDesc transition;
        };
    };

    class CommandList : public Object {
    public:
        virtual void CopyBuffer(Buffer* src, Buffer* dst, size_t srcOffset, size_t dstOffset, size_t size) noexcept = 0;
        virtual void Dispatch(const DispatchDesc& desc) noexcept = 0;
        virtual void DispatchRays(const DispatchRaysDesc& desc) noexcept = 0;
        virtual void Draw() noexcept = 0;
        virtual void ResourceBarrier(const ResourceBarrierDesc& desc) noexcept = 0;
        virtual void SetComputePSO(ComputePSO* pso) noexcept = 0;

        virtual void SetRootSignature(RootSignature* rootSignature) noexcept = 0;
        virtual void SetHeap(DescriptorHeap* CBVSRVUAVHeap, DescriptorHeap* SamplerHeap) noexcept = 0;

    public:
        virtual void BuildRTPSO(RTPSO* pso) noexcept = 0;
        virtual BLAS* BuildBLAS(const BLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                const char* name) noexcept = 0;
        virtual TLAS* BuildTLAS(const TLASDesc& desc, Buffer* scratchBuffer, size_t scratchBufferStartOffset,
                                const char* name) noexcept = 0;
    };

    struct WriteBufferDescriptorDesc {
        Buffer* buffer = nullptr;
        size_t bufferStructuredStride = 0;
        size_t size = 0;
        size_t bufferOffset = 0;
        size_t offsetInHeap = 0;
    };

    struct WriteTexture2DDescriptorDesc {
        Texture2D* texture = nullptr;
        size_t offsetInHeap = 0;
    };

    struct WriteTexture3DDescriptorDesc {
        Texture2D* texture = nullptr;
        size_t offsetInHeap = 0;
    };

    struct WriteTLASDescriptorDesc {
        TLAS* tlas = nullptr;
        size_t offsetInHeap = 0;
    };

    class DescriptorHeap : public Object {
    public:
        virtual void WriteSRV(const WriteBufferDescriptorDesc& desc) noexcept = 0;
        virtual void WriteUAV(const WriteBufferDescriptorDesc& desc) noexcept = 0;
        virtual void WriteCBV(const WriteBufferDescriptorDesc& desc) noexcept = 0;

        virtual void WriteSRV(const WriteTexture2DDescriptorDesc& desc) noexcept = 0;
        virtual void WriteUAV(const WriteTexture2DDescriptorDesc& desc) noexcept = 0;

        virtual void WriteSRV(const WriteTexture3DDescriptorDesc& desc) noexcept = 0;
        virtual void WriteUAV(const WriteTexture3DDescriptorDesc& desc) noexcept = 0;

        virtual void WriteSRV(const WriteTLASDescriptorDesc& desc) noexcept = 0;
    };
} // namespace RHINO

RHINO_DECLARE_BITMASK_ENUM(RHINO::ResourceUsage);
