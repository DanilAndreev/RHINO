#pragma once

#include "CompilationChain.h"

namespace SCAR {
    class HlslToSpirvStep final : public CompilationChain {
        bool Execute(const CompileSettings& settings, const ChainSettings& chSettings,
                     ChainContext& context) noexcept final;
    };
} // namespace SCAR
