#include "SerializeRootSignature.h"

namespace SCAR {
    SCAR_FORCEINLINE size_t CalculateSerializedRangesOffset(const PSORootSignatureDesc& rootSignature) noexcept {
        return sizeof(PSORootSignatureDesc) + rootSignature.spacesCount * sizeof(RHINO::DescriptorSpaceDesc);
    }

    SCAR_FORCEINLINE size_t CalculateSerializedSize(const PSORootSignatureDesc& rootSignature) noexcept {
        size_t result = CalculateSerializedRangesOffset(rootSignature);
        for (size_t space = 0; space < rootSignature.spacesCount; ++space) {
            result += sizeof(RHINO::DescriptorRangeDesc) * rootSignature.spacesDescs[space].rangeDescCount;
        }
        return result;
    }

    std::vector<uint8_t> SerializeRootSignature(const PSORootSignatureDesc& inputRootSignature) noexcept {
        std::vector<uint8_t> result{};
        result.resize(CalculateSerializedSize(inputRootSignature));

        constexpr size_t spacesOffsetInBytes = sizeof(PSORootSignatureDesc);
        const size_t rangesOffsetInBytes = CalculateSerializedRangesOffset(inputRootSignature);

        auto* rootSignature = reinterpret_cast<PSORootSignatureDesc*>(result.data());
        auto* spacesDescs = reinterpret_cast<RHINO::DescriptorSpaceDesc*>(result.data() + spacesOffsetInBytes);
        auto* rangeDescs = reinterpret_cast<RHINO::DescriptorRangeDesc*>(result.data() + rangesOffsetInBytes);

        size_t rangesOffset = 0;
        for (size_t space = 0; space < inputRootSignature.spacesCount; ++space) {
            spacesDescs[space] = inputRootSignature.spacesDescs[space];
            const size_t offset = rangesOffsetInBytes + rangesOffset * sizeof(RHINO::DescriptorRangeDesc);
            spacesDescs[space].rangeDescs = reinterpret_cast<RHINO::DescriptorRangeDesc*>(offset);
            for (size_t i = 0; i < inputRootSignature.spacesDescs[space].rangeDescCount; ++i) {
                rangeDescs[rangesOffset] = inputRootSignature.spacesDescs[space].rangeDescs[i];
                ++rangesOffset;
            }
        }

        *rootSignature = inputRootSignature;
        rootSignature->spacesDescs = reinterpret_cast<RHINO::DescriptorSpaceDesc*>(spacesOffsetInBytes);

        return result;
    }
} // namespace SCAR
