#include "SCARComputePSOArchiveView.h"
#define SCAR_RHINO_ADDONS
#include <SCARUnarchiver.h>


#include "SCAR.h"


namespace RHINO::SCARTools {
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

        m_RootSignatureView = reader.CreateRootSignatureView();
        m_Desc.spacesCount = m_RootSignatureView.size();
        m_Desc.spacesDescs = m_RootSignatureView.data();

        m_Desc.debugName = debugName;
    }

    const ComputePSODesc& SCARComputePSOArchiveView::GetDesc() const noexcept { return m_Desc; }
    bool SCARComputePSOArchiveView::IsValid() const noexcept { return m_IsValid; }
} // namespace RHINO::SCARTools
