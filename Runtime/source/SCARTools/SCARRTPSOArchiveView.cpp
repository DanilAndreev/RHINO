#include "SCARRTPSOArchiveView.h"

#define SCAR_RHINO_ADDONS
#include <SCARUnarchiver.h>

namespace RHINO::SCARTools {
    SCARRTPSOArchiveView::SCARRTPSOArchiveView(const void* archive, uint32_t sizeInBytes, const RTPSODesc& desc) noexcept {
        SCAR::ArchiveReader reader{archive, sizeInBytes};
        reader.Read();
        if (!reader.HasRecord(SCAR::RecordType::LibAssembly)) {
            m_IsValid = false;
            return;
        }
        if (!reader.HasRecord(SCAR::RecordType::RootSignature)) {
            m_IsValid = false;
            return;
        }

        SCAR::Record lib = reader.GetRecord(SCAR::RecordType::LibAssembly);

        m_Desc.CS.entrypoint = "main";
        m_Desc.CS.bytecode = cs.data;
        m_Desc.CS.bytecodeSize = cs.dataSize;

        m_RootSignatureView = reader.CreateRootSignatureView();
        m_Desc.spacesCount = m_RootSignatureView.size();
        m_Desc.spacesDescs = m_RootSignatureView.data();

        m_Desc.debugName = debugName;
    }

    const RTPSODesc& SCARRTPSOArchiveView::GetPatchedDesc() const noexcept { return m_Desc; }
    bool SCARRTPSOArchiveView::IsValid() const noexcept { return m_IsValid; }
} // namespace RHINO::SCARTools
