#include "DebugLayer.h"

#ifdef WIN32
#include <windows.h>
#endif

namespace RHINO::DebugLayer {
    using namespace std::string_literals;
    DebugLayer::DebugLayer(RHINOInterface* wrapped) noexcept : m_Wrapped(wrapped) {}

    void DebugLayer::Initialize() noexcept {
        m_Wrapped->Initialize();
    }

    void DebugLayer::Release() noexcept {
        for (auto [handle, metaC] : m_ResourcesMeta) {
            using namespace std::string_literals;
            switch (*metaC.type) {
                case DLResourceType::Buffer: {
                    auto* meta = static_cast<BufferMeta*>(metaC.meta);
                    DW("Memory leak. Unreleased Buffer resource on RHI instance shutdown. Resource: " + meta->name);
                    break;
                }
                case DLResourceType::Texture2D: {
                    auto* meta = static_cast<BufferMeta*>(metaC.meta);
                    DW("Memory leak. Unreleased Texture2D resource on RHI instance shutdown. Resource: " + meta->name);
                    break;
                }
                case DLResourceType::Texture3D: {
                    auto* meta = static_cast<BufferMeta*>(metaC.meta);
                    DW("Memory leak. Unreleased Texture3D resource on RHI instance shutdown. Resource: " + meta->name);
                    break;
                }
                case DLResourceType::DescriptorHeap: {
                    auto* meta = static_cast<BufferMeta*>(metaC.meta);
                    DW("Memory leak. Unreleased DescriptorHeap resource on RHI instance shutdown. Resource: " + meta->name);
                    break;
                }
                case DLResourceType::CommandList: {
                    auto* meta = static_cast<BufferMeta*>(metaC.meta);
                    DW("Memory leak. Unreleased CommandList resource on RHI instance shutdown. Resource: " + meta->name);
                    break;
                }
                case DLResourceType::ComputePSO: {
                    auto* meta = static_cast<BufferMeta*>(metaC.meta);
                    DW("Memory leak. Unreleased ComputePSO resource on RHI instance shutdown. Resource: " + meta->name);
                    break;
                }
                case DLResourceType::RTPSO: {
                    auto* meta = static_cast<BufferMeta*>(metaC.meta);
                    DW("Memory leak. Unreleased RTPSO resource on RHI instance shutdown. Resource: " + meta->name);
                    break;
                }
            }
        }

        m_ResourcesMeta.clear();

        m_Wrapped->Release();
    }

    RTPSO* DebugLayer::CompileRTPSO(const RTPSODesc& desc) noexcept {
        auto* result = m_Wrapped->CompileRTPSO(desc);

        auto* meta = new RTPSOMeta{DLResourceType::RTPSO, desc.debugName};
        m_ResourcesMeta[result] = DebugMetadata{.meta = meta};

        return result;
    }

    void DebugLayer::ReleaseRTPSO(RTPSO* pso) noexcept {
        m_Wrapped->ReleaseRTPSO(pso);
        delete static_cast<RTPSOMeta*>(m_ResourcesMeta[pso].meta);
        m_ResourcesMeta.erase(pso);
    }

    ComputePSO* DebugLayer::CompileComputePSO(const ComputePSODesc& desc) noexcept {
        auto* result = m_Wrapped->CompileComputePSO(desc);

        std::set<size_t> usedSpaces{};
        for (size_t space = 0; space < desc.spacesCount; ++space) {
            if (usedSpaces.contains(desc.spacesDescs[space].space)) {
                DB("Redefined descrioptor space ["s + std::to_string(desc.spacesDescs[space].space) + "]."s);
            }
            usedSpaces.insert(desc.spacesDescs[space].space);
            if (desc.spacesDescs[space].rangeDescCount) {
                const bool isSampler = desc.spacesDescs[space].rangeDescs[0].rangeType == DescriptorRangeType::Sampler;
                for (size_t range = 0; range < desc.spacesDescs[space].rangeDescCount; ++range) {
                    if (desc.spacesDescs[space].rangeDescs[0].rangeType != DescriptorRangeType::Sampler && isSampler) {
                        DB("Invalid descriptor range type. You can either SRV/UAV/CBV or Sampler per descriptor space. Space ["s + std::to_string(space) + "] range ["s + std::to_string(range) + "]");
                    }
                }
            } else {
                DB("Invalid descriptor space descriptor ranges count. At least one descriptor range per descriptor space is mandatory. Space ["s + std::to_string(space) + "]");
            }
        }

        auto* meta = new ComputePSOMeta{DLResourceType::ComputePSO, desc.debugName};
        m_ResourcesMeta[result] = DebugMetadata{.meta = meta};

        return result;
    }

    void DebugLayer::ReleaseComputePSO(ComputePSO* pso) noexcept {
        m_Wrapped->ReleaseComputePSO(pso);
        delete static_cast<ComputePSOMeta*>(m_ResourcesMeta[pso].meta);
        m_ResourcesMeta.erase(pso);
    }

    Buffer* DebugLayer::CreateBuffer(size_t size, ResourceHeapType heapType, ResourceUsage usage, size_t structuredStride, const char* name) noexcept {
        switch (heapType) {
            case ResourceHeapType::Default:
                break;
            case ResourceHeapType::Upload:
                switch (usage) {
                    case ResourceUsage::CopySource:
                        break;
                    default:
                        DB("Invalid usage + "s + EtoS(usage) + " +  flag with ResourceHeapType::Upload of buffer '"s + name + "'. Valid values: [ResourceUsage::CopySource].");
                }
                break;
            case ResourceHeapType::Readback:
                switch (usage) {
                    case ResourceUsage::CopyDest:
                        break;
                    default:
                        DB("Invalid usage + "s + EtoS(usage) + " +  flag with ResourceHeapType::Readback of buffer '"s + name + "'. Valid values: [ResourceUsage::CopyDest].");
                }
                break;
        }

        auto* result = m_Wrapped->CreateBuffer(size, heapType, usage, structuredStride, name);

        auto* meta = new BufferMeta{DLResourceType::Buffer, name};
        m_ResourcesMeta[result] = DebugMetadata{.meta = meta};

        return result;
    }

    void* DebugLayer::MapMemory(Buffer* buffer, size_t offset, size_t size) noexcept {
        return m_Wrapped->MapMemory(buffer, offset, size);
    }

    void DebugLayer::UnmapMemory(Buffer* buffer) noexcept {
        m_Wrapped->UnmapMemory(buffer);
    }

    void DebugLayer::ReleaseBuffer(Buffer* buffer) noexcept {
        m_Wrapped->ReleaseBuffer(buffer);
        delete static_cast<BufferMeta*>(m_ResourcesMeta[buffer].meta);
        m_ResourcesMeta.erase(buffer);
    }

    Texture2D* DebugLayer::CreateTexture2D() noexcept {
        auto* result = m_Wrapped->CreateTexture2D();

        auto* meta = new Texture2DMeta{DLResourceType::Texture2D, ""};
        m_ResourcesMeta[result] = DebugMetadata{.meta = meta};

        return result;
    }

    void DebugLayer::ReleaseTexture2D(Texture2D* texture) noexcept {
        m_Wrapped->ReleaseTexture2D(texture);
        delete static_cast<Texture2DMeta*>(m_ResourcesMeta[texture].meta);
        m_ResourcesMeta.erase(texture);
    }

    DescriptorHeap* DebugLayer::CreateDescriptorHeap(DescriptorHeapType type, size_t descriptorsCount, const char* name) noexcept {
        auto* result = m_Wrapped->CreateDescriptorHeap(type, descriptorsCount, name);

        auto* meta = new DescriptorHeapMeta{DLResourceType::DescriptorHeap, name};
        m_ResourcesMeta[result] = DebugMetadata{.meta = meta};

        return result;
    }

    void DebugLayer::ReleaseDescriptorHeap(DescriptorHeap* heap) noexcept {
        m_Wrapped->ReleaseDescriptorHeap(heap);
        delete static_cast<DescriptorHeapMeta*>(m_ResourcesMeta[heap].meta);
        m_ResourcesMeta.erase(heap);
    }

    CommandList* DebugLayer::AllocateCommandList(const char* name) noexcept {
        auto* result = m_Wrapped->AllocateCommandList(name);

        auto* meta = new CommandListMeta{DLResourceType::CommandList, name};
        m_ResourcesMeta[result] = DebugMetadata{.meta = meta};

        return result;
    }

    void DebugLayer::ReleaseCommandList(CommandList* commandList) noexcept {
        m_Wrapped->ReleaseCommandList(commandList);
        delete static_cast<CommandListMeta*>(m_ResourcesMeta[commandList].meta);
        m_ResourcesMeta.erase(commandList);
    }

    void DebugLayer::SubmitCommandList(CommandList* cmd) noexcept {
        m_Wrapped->SubmitCommandList(cmd);
    }

    void DebugLayer::DB(const std::string& text) noexcept {
        const std::string message = "\nRHINO Validation [Error]: " + text + ".\n";
#ifdef WIN32
        OutputDebugStringA(message.c_str());
        __debugbreak();
#endif// WIN32
    }

    void DebugLayer::DW(const std::string& text) noexcept {
        const std::string message = "\nRHINO Validation [Warning]: " + text  + ".\n";
#ifdef WIN32
        OutputDebugStringA(message.c_str());
#endif// WIN32
    }

#define RHINO_ENUM_SWITCH_CASE(e) case e: return #e;
    const char* DebugLayer::EtoS(ResourceUsage usage) noexcept {
        switch (usage) {
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::None)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::VertexBuffer)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::IndexBuffer)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::ConstantBuffer)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::ShaderResource)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::UnorderedAccess)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::Indirect)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::CopySource)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::CopyDest)
            RHINO_ENUM_SWITCH_CASE(ResourceUsage::ValidMask)
        }
    }
}// namespace RHINO::DebugLayer
