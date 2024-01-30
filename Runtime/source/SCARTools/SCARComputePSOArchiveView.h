#pragma once

namespace RHINO::SCARTools {
    class SCARComputePSOArchiveView {
    public:
        explicit SCARComputePSOArchiveView(const void* archive, uint32_t sizeInBytes, const char* debugName) noexcept;
        const ComputePSODesc& GetDesc() const noexcept;
        bool IsValid() const noexcept;
    private:
        ComputePSODesc m_Desc{};
        std::vector<DescriptorSpaceDesc> m_RootSignatureView{};
        bool m_IsValid = true;
    };
} // namespace RHINO::SCARTools
