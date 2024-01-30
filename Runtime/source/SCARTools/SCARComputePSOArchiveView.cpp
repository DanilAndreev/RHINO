#include "SCARComputePSOArchiveView.h"
#define SCAR_RHINO_ADDONS
#include <SCARUnarchiver.h>


#include "SCAR.h"


namespace RHINO::SCARTools {
    std::vector<DescriptorSpaceDesc> CreateRootSignatureView(const uint8_t* start) {
        const auto rootSignature = reinterpret_cast<const SCAR::PSORootSignatureDesc*>(start);

        const auto spacesOffset = reinterpret_cast<size_t>(rootSignature->spacesDescs);
        const auto* serializedSpacesDescs = reinterpret_cast<const DescriptorSpaceDesc*>(start + spacesOffset);

        std::vector<DescriptorSpaceDesc> spacesDescs{rootSignature->spacesCount};
        for (size_t space = 0; space < rootSignature->spacesCount; ++space) {
            spacesDescs[space] = serializedSpacesDescs[space];
            const auto rangesOffset = reinterpret_cast<size_t>(spacesDescs[space].rangeDescs);
            spacesDescs[space].rangeDescs = reinterpret_cast<const DescriptorRangeDesc*>(start + rangesOffset);
        }
        return spacesDescs;
    }

    SCARComputePSOArchiveView::SCARComputePSOArchiveView(const void* archive, uint32_t sizeInBytes,
                                                         const char* debugName) noexcept {
        SCAR::ArchiveReader reader{archive, sizeInBytes};
        reader.Read();
        if (!reader.HasRecord(SCAR::RecordType::CSAssembly)) {
            m_IsValid = false;
            return;
        }
        if (!reader.HasRecord(SCAR::RecordType::RootSignature)) {
            m_IsValid = false;
            return;
        }

        SCAR::Record cs = reader.GetRecord(SCAR::RecordType::CSAssembly);
        // TODO: read from archive
        m_Desc.CS.entrypoint = "main";
        m_Desc.CS.bytecode = cs.data;
        m_Desc.CS.bytecodeSize = cs.dataSize;

        const SCAR::Record rootSignature = reader.GetRecord(SCAR::RecordType::RootSignature);
        m_RootSignatureView = CreateRootSignatureView(rootSignature.data);
        m_Desc.spacesCount = m_RootSignatureView.size();
        m_Desc.spacesDescs = m_RootSignatureView.data();

        m_Desc.debugName = debugName;
    }

    const ComputePSODesc& SCARComputePSOArchiveView::GetDesc() const noexcept { return m_Desc; }
    bool SCARComputePSOArchiveView::IsValid() const noexcept { return m_IsValid; }
} // namespace RHINO::SCARTools
