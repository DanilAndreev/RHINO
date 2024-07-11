#pragma once

#include <SCARUnarchiver.h>

namespace SCAR {
    std::vector<uint8_t> SerializeRootSignature(const PSORootSignatureDesc& rootSignature) noexcept;
    std::vector<uint8_t> SerializeEntrypoints(const std::vector<std::string>& entrypoints) noexcept;
}