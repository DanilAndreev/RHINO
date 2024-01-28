#pragma once

#include <string>

namespace RHINO::DebugLayer {
    enum class DLResourceType : size_t {
        Buffer,
        Texture2D,
        Texture3D,
        DescriptorHeap,
        CommandList,
        ComputePSO,
        RTPSO,
    };

    union DebugMetadata {
        DLResourceType* type;
        void* meta;
    };

    struct BufferMeta {
        DLResourceType type = DLResourceType::Buffer;
        std::string name = "UnnamedBuffer";
    };

    struct Texture2DMeta {
        DLResourceType type = DLResourceType::Texture2D;
        std::string name = "UnnamedTexture2D";
    };

    struct Texture3DMeta {
        DLResourceType type = DLResourceType::Texture3D;
        std::string name = "UnnamedTexture3D";
    };

    struct DescriptorHeapMeta {
        DLResourceType type = DLResourceType::DescriptorHeap;
        std::string name = "UnnamedDescriptorHeap";
    };

    struct CommandListMeta {
        DLResourceType type = DLResourceType::CommandList;
        std::string name = "UnnamedCommandList";
    };

    struct ComputePSOMeta {
        DLResourceType type = DLResourceType::ComputePSO;
        std::string name = "UnnamedComputePSO";
    };

    struct RTPSOMeta {
        DLResourceType type = DLResourceType::RTPSO;
        std::string name = "UnnamedRTPSO";
    };

    class DebugLayer final : public RHINOInterface {
    public:
        explicit DebugLayer(RHINOInterface* wrapped) noexcept;

    public:
        void Initialize() noexcept final;
        void Release() noexcept final;
        RTPSO* CompileRTPSO(const RTPSODesc& desc) noexcept final;
        void ReleaseRTPSO(RTPSO* pso) noexcept final;
        ComputePSO* CompileComputePSO(const ComputePSODesc& desc) noexcept final;
        void ReleaseComputePSO(ComputePSO* pso) noexcept final;
        Buffer* CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept final;
        void* MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept final;
        void UnmapMemory(Buffer* buffer) noexcept final;
        void ReleaseBuffer(Buffer* buffer) noexcept final;
        Texture2D* CreateTexture2D() noexcept final;
        void ReleaseTexture2D(Texture2D* texture) noexcept final;
        DescriptorHeap* CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept final;
        void ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept final;
        CommandList* AllocateCommandList(const char* name) noexcept final;
        void ReleaseCommandList(CommandList* commandList) noexcept final;
        void SubmitCommandList(CommandList* cmd) noexcept final;
    private:
        /**
         * Debug break;
         */
        static void DB(const std::string& text) noexcept;
        /**
         * Debug warning.
         */
        static void DW(const std::string& text) noexcept;

        // Enum to String
        static const char* EtoS(ResourceUsage usage) noexcept;

    private:
        RHINOInterface* m_Wrapped = nullptr;
        std::map<void*, DebugMetadata> m_ResourcesMeta{};
    };
}// namespace RHINO::DebugLayer
