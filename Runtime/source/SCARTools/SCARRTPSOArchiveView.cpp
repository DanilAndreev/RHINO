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
        if (!reader.HasRecord(SCAR::RecordType::ShadersEntrypoints)) {
            m_IsValid = false;
            return;
        }

        const SCAR::Record& libAss = reader.GetRecord(SCAR::RecordType::LibAssembly);

        for (const char* entry : reader.CreateEntrypointsView()) {
            ShaderModule shaderModule{};
            shaderModule.bytecodeSize = libAss.dataSize;
            shaderModule.bytecode = libAss.data;
            shaderModule.entrypoint = entry;
            m_ShaderModulesView.emplace_back(shaderModule);
        }

        m_Desc = desc;
        m_Desc.shaderModulesCount = m_ShaderModulesView.size();
        m_Desc.shaderModules = m_ShaderModulesView.data();

        // m_RootSignatureView = reader.CreateRootSignatureView();
        // m_Desc.spacesCount = m_RootSignatureView.size();
        // m_Desc.spacesDescs = m_RootSignatureView.data();
    }

    const RTPSODesc& SCARRTPSOArchiveView::GetPatchedDesc() const noexcept { return m_Desc; }
    bool SCARRTPSOArchiveView::IsValid() const noexcept { return m_IsValid; }
} // namespace RHINO::SCARTools
