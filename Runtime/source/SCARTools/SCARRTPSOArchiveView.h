#pragma once

namespace RHINO::SCARTools {
    class SCARRTPSOArchiveView {
    public:
        explicit SCARRTPSOArchiveView(const void* archive, uint32_t sizeInBytes, const RTPSODesc& desc) noexcept;
        const RTPSODesc& GetPatchedDesc() const noexcept;
        bool IsValid() const noexcept;

    private:
        RTPSODesc m_Desc{};
        std::vector<ShaderModule> m_ShaderModulesView{};
        std::vector<DescriptorSpaceDesc> m_RootSignatureView{};
        bool m_IsValid = true;
    };
} // namespace RHINO::SCARTools
