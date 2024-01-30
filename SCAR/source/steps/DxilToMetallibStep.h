#pragma once

#include "CompilationChain.h"

namespace SCAR {
    class DxilToMetallibStep final : public CompilationChain {
    public:
        bool Execute(const CompileSettings& settings, const ChainSettings& chSettings,
                     ChainContext& context) noexcept final;
    };
} // namespace SCAR
